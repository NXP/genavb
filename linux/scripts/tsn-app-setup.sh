#!/bin/sh

if [ $# -ne 1 ]; then
	echo "$(basename "$0") <interface>"
	exit 1
fi

. "/etc/genavb/config_common"

# Source common genavb functions
. "/etc/genavb/common_func.sh"

ITF=$1

NB_CPU=$(grep -c processor /proc/cpuinfo)
NB_CPU_MASK="$(( (1 << NB_CPU) - 1))"

if  [ "$NB_CPU" -gt 2 ];then
	TSN_APP_CPU_CORE="${TSN_APP_CORE_GT2C}"
elif  [ "$NB_CPU" -eq 2 ];then
	TSN_APP_CPU_CORE="${TSN_APP_CORE_2C}"
else
	TSN_APP_CPU_CORE="${TSN_APP_CORE_1C}"
fi

TSN_APP_CPU_MASK="$(( 1 << TSN_APP_CPU_CORE ))"

# Default driver settings
HAS_PER_QUEUE_COALESCING=0
HAS_TC_FLOWER_VLAN_CLASSIFICATION_OFFLOAD=0
HAS_ONE_IRQ_PER_RING_BUFFER=0
HAS_XDP_ZERO_COPY_SUPPORT=0
HAS_SEPARATE_TX_RX_NAPI_THREADS=0
HAS_GPTP_RX_CLASSIFICATION=0

. "/etc/genavb/config_tsn"

if [ ! -e "$APPS_CFG_FILE" ]; then
	echo "TSN app configuration file '$APPS_CFG_FILE' not found"
	exit 1
fi

. "$APPS_CFG_FILE"

set_machine_variables()
{
# Some platform-dependant variables.
case $1 in
'imx8mpevk'|'imx93evk'|'imx8dxlevk'|'imx93evkauto'|'imx93qsb')
	# Number of TX/RX queues
	NUM_TC=${ENETQOS_TSN_ENDPOINT_NUM_TC}

	# EnetQos driver has per queue coalescing settings
	HAS_PER_QUEUE_COALESCING=1

	# EnetQos driver has TC flower vlan classification offload
	HAS_TC_FLOWER_VLAN_CLASSIFICATION_OFFLOAD=1

	# EnetQos driver has one IRQ for all queues
	HAS_ONE_IRQ_PER_RING_BUFFER=0

	HAS_XDP_ZERO_COPY_SUPPORT=1

	# Has napi thread per ring (tx and rx are separate threads)
	HAS_SEPARATE_TX_RX_NAPI_THREADS=1

	HAS_GPTP_RX_CLASSIFICATION=0

	;;
'imx95evk')
	# Number of TX/RX queues
	NUM_TC=${IMX95EVK_TSN_ENDPOINT_NUM_TC}

	# enetc driver supports psfp offload with tc flower
	HAS_TC_FLOWER_PSFP_OFFLOAD=1

	HAS_GPTP_RX_CLASSIFICATION=1

	# enetc has one IRQ per TX/RX ring buffer, get the IRQ for TSN ringbuffer
	HAS_ONE_IRQ_PER_RING_BUFFER=1

	# NAPI Configuration
	HAS_XDP_ZERO_COPY_SUPPORT=0
	# Has napi thread per tx/rx ring (tx and rx are processed on the same thread)
	HAS_SEPARATE_TX_RX_NAPI_THREADS=0
	;;
*)
	echo "Error: Unsupported machine $1"
	exit 1
	;;
esac
}

# Return a string containing all XDP ZC napi tasks for a specific traffic class number
# $1: traffic class (e.g Hw queue) number
get_napi_xdp_zc_tasks_per_tc()
{
	if [ $HAS_XDP_ZERO_COPY_SUPPORT -eq "1" ]; then
		echo "napi/$ITF-zc-$1"
	else
		echo ""
	fi
}

# Return a string containing all napi tasks for a specific traffic class number
# $1: prefix (rx, tx, or rxtx)
# $2: traffic class (e.g Hw queue) number
get_napi_tasks_per_tc()
{
	if [ $HAS_SEPARATE_TX_RX_NAPI_THREADS -ne "1" ]; then
		echo "napi/$ITF-rxtx-$2"
	else
		if [ "$1" = "rxtx" ]; then
			echo "napi/$ITF-tx-$2 napi/$ITF-rx-$2"
		else
			echo "napi/$ITF-$1-$2"
		fi
	fi
}

set_tsn_configurations_variables()
{
	if [ "$NUM_TC" -ne "5" ] && [ "$NUM_TC" -ne "6" ]; then
		echo "Error: Unsupported number of HW traffic classes"
		exit 1
	fi

	if [ "$NUM_TC" -eq "5" ]; then
		# PCP mapping (802.1Q Table 8-5, no SR Class, 5 traffic classes):
		#    * prio {0, 1, 8-15}              -> TC 0
		#    * prio {2, 3}                    -> TC 1
		#    * prio {4, 5 (tsn app traffic)}  -> TC 2
		#    * prio 6 (gptp/srp traffic)      -> TC 3
		#    * prio 7                         -> TC 4
		PCP_TO_QOS_MAP="${PCP_TO_QOS_MAP_5TC_0SR}"
	elif [ "$NUM_TC" -eq "6" ]; then
		# PCP mapping (802.1Q Table 8-5, no SR Class, 6 traffic classes):
		#    * prio {0, 8-15}                 -> TC 1
		#    * prio {1}                       -> TC 0
		#    * prio {2, 3}                    -> TC 2
		#    * prio {4, 5 (tsn app traffic)}  -> TC 3
		#    * prio 6 (gptp/srp traffic)      -> TC 4
		#    * prio 7                         -> TC 5
		PCP_TO_QOS_MAP="${PCP_TO_QOS_MAP_6TC_0SR}"
	fi

	NUM_TC_MASK="$(( (1 << NUM_TC) - 1))"
	HW_QUEUES_MAPPING="$(get_one2one_hw_queues_mapping "$NUM_TC")" # 1:1 mapping between HW queues and traffic classes
	TSN_TRAFFIC_TC="$(get_list_index_val "$PCP_TO_QOS_MAP" "$TSN_TRAFFIC_PRIO")"
	GPTP_TX_TRAFFIC_TC="$(get_list_index_val "$PCP_TO_QOS_MAP" "$GPTP_TRAFFIC_PRIO")"

	if [ $HAS_GPTP_RX_CLASSIFICATION -eq "1" ]; then
		GPTP_RX_TRAFFIC_TC="${GPTP_TX_TRAFFIC_TC}"     # Same TC for gPTP RX and TX traffic
	else
		GPTP_RX_TRAFFIC_TC=0     # TC 0 for gPTP RX traffic
	fi

	echo "Configured HW traffic classes: $NUM_TC"
	echo "    TSN :  TX --> TC#$TSN_TRAFFIC_TC | RX --> TC#$TSN_TRAFFIC_TC"
	echo "    GPTP:  TX --> TC#$GPTP_TX_TRAFFIC_TC | RX --> TC#$GPTP_RX_TRAFFIC_TC"

	TSN_GATE_MASK=0x$((1 << TSN_TRAFFIC_TC))
	NON_TSN_GATE_MASK=$(printf "%x" $((~TSN_GATE_MASK & NUM_TC_MASK)))
	TSN_GATE_NS=4000 # gate window in ns

	# Get TSN IRQ number
	if [ $HAS_ONE_IRQ_PER_RING_BUFFER -eq "1" ]; then
		ITF_TSN_IRQ=$(grep "$ITF-rxtx$TSN_TRAFFIC_TC" /proc/interrupts | awk -F: '{ print $1 }')
	else
		ITF_TSN_IRQ=$(grep "$ITF" /proc/interrupts | awk -F: '{ print $1 }')
	fi

	TSN_TRAFFIC_NAPI_TASKS="$(get_napi_tasks_per_tc "rxtx" "$TSN_TRAFFIC_TC")"
	TSN_TRAFFIC_NAPI_XDP_ZC_TASKS="$(get_napi_xdp_zc_tasks_per_tc "$TSN_TRAFFIC_TC")"

	GPTP_TRAFFIC_NAPI_TASKS="$(get_napi_tasks_per_tc "tx" "$GPTP_TX_TRAFFIC_TC")"

	# If no gPTP RX classification and the driver has one napi thread per tx/rx ring, get the gptp rx napi thread
	if [ $HAS_GPTP_RX_CLASSIFICATION -ne 1 ] && [ $HAS_SEPARATE_TX_RX_NAPI_THREADS -eq 1 ]; then
		GPTP_TRAFFIC_NAPI_TASKS="${GPTP_TRAFFIC_NAPI_TASKS} $(get_napi_tasks_per_tc "rx" "$GPTP_RX_TRAFFIC_TC")"
	fi

	# All non-TSN and non-gPTP napi tasks are treated as best effort
	BEST_EFFORT_NAPI_TASKS=""
	for tc in $(seq 0 $(( NUM_TC - 1))); do
		# Skip TSN traffic queues (tx, rx and zc)
		[ "$tc" -eq "$TSN_TRAFFIC_TC" ] && continue

		# All non TSN traffic AF XDP zero copy are best effort (gPTP does not use AF_XDP)
		if [ "$HAS_XDP_ZERO_COPY_SUPPORT" -eq 1 ]; then
			BEST_EFFORT_NAPI_TASKS="${BEST_EFFORT_NAPI_TASKS} $(get_napi_xdp_zc_tasks_per_tc "$tc")"
		fi

		# If gPTP TX and RX are on different TCs, mark the opposite queue as best effort
		if [ "$tc" = "$GPTP_TX_TRAFFIC_TC" ]; then
			[ $HAS_GPTP_RX_CLASSIFICATION -ne 1 ] && BEST_EFFORT_NAPI_TASKS="${BEST_EFFORT_NAPI_TASKS} $(get_napi_tasks_per_tc "rx" "$tc")"
		elif [ "$tc" = "$GPTP_RX_TRAFFIC_TC" ]; then
			[ $HAS_GPTP_RX_CLASSIFICATION -ne 1 ] && BEST_EFFORT_NAPI_TASKS="${BEST_EFFORT_NAPI_TASKS} $(get_napi_tasks_per_tc "rx" "$tc")"
		else
			BEST_EFFORT_NAPI_TASKS="${BEST_EFFORT_NAPI_TASKS} $(get_napi_tasks_per_tc "rxtx" "$tc")"
		fi

	done
}

# Extract Qbv config from the application options
echo "$CFG_EXTERNAL_MEDIA_APP_OPT" | grep -q -e "-r[[:space:]]\+io_device" && IODEVICE="iodevice" || IODEVICE=""
PERIOD_NS=$(echo "$CFG_EXTERNAL_MEDIA_APP_OPT" | grep -o "\-p[[:space:]]\+[0-9]\+" | grep -o "[0-9]\+")

# Check if application is in AF_XDP mode
echo "$CFG_EXTERNAL_MEDIA_APP_OPT" | grep -q -e "-x" && AF_XDP_MODE="true" || AF_XDP_MODE="false"

# Default Period
[ "$PERIOD_NS" = "" ] && PERIOD_NS=2000000

# Set right Qbv start phase shift
SW_PROCESSING_BUDGET=$((PERIOD_NS / 4))
if [ "$IODEVICE" = "iodevice" ]; then
        # IO device
        QBV_START_PHASE_SHIFT_NS=$((PERIOD_NS / 2 + SW_PROCESSING_BUDGET))
else
        # Controller device
        QBV_START_PHASE_SHIFT_NS=$((SW_PROCESSING_BUDGET))
fi

# $1 is pre delay in seconds
# $2 is timeout in seconds
wait_link_up()
{
	sleep "$1"
	timeout "$2" sh -c "while ! cat /sys/class/net/$ITF/operstate | grep -q up; do sleep 1;done"
}

set_vlan_config()
{
	echo "# Setup VLAN 2 on interface $ITF"

	# Enable interface and setup VLAN 2 (VLAN hardware filtering is enabled by
	# default on recent kernels)
	ip link add link "$ITF" name vlan0 type vlan id 2
	ip link set dev vlan0 up
}

load_xdp_program()
{
	if [ "${AF_XDP_MODE}" = "true" ]; then
		xdp_prog_loaded=$(ip link show dev "$ITF" | grep xdp)
		if [ -z "${xdp_prog_loaded}" ]; then
			echo "# Load XDP program on interface $ITF"

			ip link set dev "$ITF" xdp obj /usr/lib/firmware/genavb/genavb-xdp.bin
			sleep 1
		fi
	fi
}

set_interface_low_latency_settings()
{
	# Disable coalescing on TSN queue
	if [ "$HAS_PER_QUEUE_COALESCING" -eq 1 ]; then
		echo "# Disable coalescing on TSN queue ($TSN_TRAFFIC_TC) and flow control on interface $ITF"
		ethtool --per-queue "$ITF" queue_mask "$TSN_GATE_MASK" --coalesce rx-usecs 16 tx-usecs 10000 tx-frames 1
	else
		echo " Per queue coalescing setting is not available ... skip"
	fi

	# Disable flow control
	ethtool -A "$ITF" autoneg off rx off tx off
	# Changing pause parameters would cause link state changes, wait for link up.
	wait_link_up "1" "5"
}

setup_tx_scheduling_and_rx_classification_settings()
{
	echo "# Setup taprio qdisc on interface $ITF"
	# Setup 802.1Qbv (using tc taprio qdisc).
	tc qdisc del dev "$ITF" root

	# Setup 802.1Qbv (using tc taprio qdisc).
	# shellcheck disable=SC2086 # Intentional splitting of options for map and queues
	tc qdisc replace dev "$ITF" root taprio \
		num_tc "${NUM_TC}" \
		map ${PCP_TO_QOS_MAP} \
		queues ${HW_QUEUES_MAPPING} \
		base-time "$QBV_START_PHASE_SHIFT_NS" \
		sched-entry S "$TSN_GATE_MASK" "$TSN_GATE_NS" \
		sched-entry S "$NON_TSN_GATE_MASK" $((PERIOD_NS - TSN_GATE_NS)) \
		flags 0x2

	# Setup Rx classification (using tc flower qdisc).
	# Must be called after taprio_config (depends on
	# traffic class definition from taprio).
	if ! cat /lib/modules/$(uname -r)/modules.builtin | grep -q cls_flower; then
		echo "insert cls_flower kernel module"
		modprobe cls_flower
	fi

	tc qdisc add dev "$ITF" ingress
	tc filter del dev "$ITF" ingress

	if [ "$HAS_TC_FLOWER_VLAN_CLASSIFICATION_OFFLOAD" -eq 1 ]; then
		echo "# Setup RX classification using TC filter flower with vlan classification offload on interface $ITF"
		# Incoming TSN app traffic goes into same TC (HW queue) as the TX
		tc filter add dev "$ITF" parent ffff: protocol 802.1Q flower vlan_prio "${TSN_TRAFFIC_PRIO}" hw_tc "${TSN_TRAFFIC_TC}"
		# Incoming Tagged best effort traffic to TC 1
		tc filter add dev "$ITF" parent ffff: protocol 802.1Q flower vlan_prio 0 hw_tc 1

	elif [ "$HAS_TC_FLOWER_PSFP_OFFLOAD" -eq 1 ]; then

		if [ "$MACHINE" = "imx95evk" ]; then
			echo "# Offload priority to ring buffer mapping using Station interface IPV to ring mapping register (SIIPVBDRMR0)"
			/unit_tests/memtool -32 0x4CC00150=0x54332201
		fi

		echo "# Setup RX classification using TC filter flower with PSFP offload on interface $ITF"

		# chain index 0: gPTP traffic
		# gate index 1: open gate and IPV overriden to 6 without rate policing
		tc filter add dev "$ITF" parent ffff: protocol 802.1q chain 0 \
			flower skip_sw dst_mac "${GPTP_DST_MAC}" \
			action gate index 1 \
			sched-entry OPEN 1000000000 "${GPTP_TRAFFIC_PRIO}" -1

		# offloading multiple tc flower to hardware successively without delay can cause input/output errors.
		sleep 1

		# chain index 1: TSN traffic from controller
		# gate index 2: open gate and IPV overriden to 3 without rate policing
		tc filter add dev "$ITF" parent ffff: protocol 802.1q chain 1 \
			flower skip_sw dst_mac "${TSN_CONTROLLER_TRAFFIC_DST_MAC}" vlan_id "${TSN_TRAFFIC_VLAN_ID}" \
			action gate index 2 \
			sched-entry OPEN 1000000000 "${TSN_TRAFFIC_PRIO}" -1

		sleep 1

		# chain index 2: TSN traffic from iodevice 1
		# Use same gate index 2
		tc filter add dev "$ITF" parent ffff: protocol 802.1q chain 2 \
			flower skip_sw dst_mac "${TSN_IODEVICE1_TRAFFIC_DST_MAC}" vlan_id "${TSN_TRAFFIC_VLAN_ID}" \
			action gate index 2

		sleep 1

		# chain index 3: TSN traffic from iodevice 2
		# Use same gate index 2
		tc filter add dev "$ITF" parent ffff: protocol 802.1q chain 3 \
			flower skip_sw dst_mac "${TSN_IODEVICE2_TRAFFIC_DST_MAC}" vlan_id "${TSN_TRAFFIC_VLAN_ID}" \
			action gate index 2

		sleep 1

	else
		echo "Warning: unable to set proper RX classification settings."
	fi

}

isolate_tsn_app_cpu_core()
{
	echo "# Move all processes off CPU core $TSN_APP_CPU_CORE used for tsn-app"

	# Move processes off TSN app core. Ignore errors
	# for unmoveable processes.
	# pgrep has a 15 characters limitation
	# shellcheck disable=SC2009
	for i in $(ps aux | grep -v PID | grep -v napi/"$ITF" | awk '{print $2;}'); do
		if ! ps -p "$i" > /dev/null; then
			# Not a running process, skip
			continue;
		fi

		curr_affinity="0x$(taskset -p "$i" | cut -d ":" -f2 | tr -d "[:space:]")"
		new_affinity="$(printf "%x" $((curr_affinity & ~TSN_APP_CPU_MASK)))";

		taskset -p "$new_affinity" "$i" > /dev/null 2>&1;
	done

	_wq_cpu_mask="$(printf "%x" $((NB_CPU_MASK & ~TSN_APP_CPU_MASK)))"
	echo "Move workqueues off TSN CPU core: Use cpumask ($_wq_cpu_mask) for them"
	find /sys/devices/virtual/workqueue -name cpumask -exec sh -c 'wq_mask="$1"; wq_path="$2"; echo $wq_mask > $wq_path' shell "$_wq_cpu_mask" {} \;
}

# pgrep has a 15 charecters name limit, use this one for long task/process names
# $1: process name
get_pid_from_process_name()
{
	ps_list="$(${PS})"
	_pid="$(echo "$ps_list" | grep "$1" | awk '{print $1}')"

	echo "${_pid}"
}

set_napi_and_irqs_priority_and_affinities()
{
	echo "# Enable threaded NAPI"
	# Configure real-time priorities and CPU affinities.
	# Enable threaded NAPI.
	echo 1 > /sys/class/net/"$ITF"/threaded

	# Move all best-effort/non-critical queues
	if  [ "$NB_CPU" -gt 2 ];then
		# to CPU core 3 (4th core)
		best_effort_cpu_mask=8
	else
		# to CPU core 0 (1st core)
		best_effort_cpu_mask=1
	fi

	for napi_task_name in $BEST_EFFORT_NAPI_TASKS
	do
		napi_task_pid=$(get_pid_from_process_name "$napi_task_name")

		echo "# Move NAPI task ($napi_task_name ($napi_task_pid)) to Best Effort CPU"
		taskset -p "$best_effort_cpu_mask" "$napi_task_pid"
	done

	# Ethernet IRQ on
	if  [ "$NB_CPU" -gt 2 ];then
		# CPU core 1 (2nd core)
		eth_irq_cpu_mask=2
	else
		# CPU core 0 (1st core)
		eth_irq_cpu_mask=1
	fi

	# High prio to ensure high prio packets
	# are handled quickly.
	for irq in $ITF_TSN_IRQ; do
		irq_thread_pid="$(pgrep irq/"$irq"-"$ITF")"

		echo "# Move TSN IRQ ($irq) thread ($irq_thread_pid) to TSN CPU with RT prio 66"

		taskset -p "$eth_irq_cpu_mask" "$irq_thread_pid"
		echo "$eth_irq_cpu_mask" > /proc/irq/"$irq"/smp_affinity
		chrt -pf 66 "$irq_thread_pid"
	done

	# gPTP queues on
	if  [ "$NB_CPU" -gt 2 ];then
		# CPU core 1 (2nd core)
		gptp_cpu_mask=2
	else
		# CPU core 0 (1st core)
		gptp_cpu_mask=1
	fi

	for napi_task_name in $GPTP_TRAFFIC_NAPI_TASKS; do
		napi_task_pid=$(get_pid_from_process_name "$napi_task_name")

		echo "# Move gPTP NAPI task ($napi_task_name ($napi_task_pid)) to TSN CPU with RT prio 1"
		taskset -p "$gptp_cpu_mask" "$napi_task_pid"
		chrt -pf 1 "$napi_task_pid"
	done

	# TSN traffic queues on same core as the tsn app
	# same prio as tsn-app for AF_XDP queue
	# in AF_PACKET mode, give network queues a
	# higher prio than tsn-app
	for napi_task_name in $TSN_TRAFFIC_NAPI_TASKS; do
		napi_task_pid=$(get_pid_from_process_name "$napi_task_name")

		echo "# Move TSN NAPI task ($napi_task_name ($napi_task_pid)) to TSN CPU with RT prio 61"
		taskset -p "$TSN_APP_CPU_MASK" "$napi_task_pid"
		chrt -pf 61 "$napi_task_pid"
	done

	for napi_task_name in $TSN_TRAFFIC_NAPI_XDP_ZC_TASKS; do
		napi_task_pid=$(get_pid_from_process_name "$napi_task_name")

		echo "# Move TSN NAPI XDP ZC task ($napi_task_name ($napi_task_pid)) to TSN CPU with RT prio 60"
		taskset -p "$TSN_APP_CPU_MASK" "$napi_task_pid"
		chrt -pf 60 "$napi_task_pid"
	done
}

############################ MAIN ############################

# Detect the platform we are running on and set $MACHINE accordingly, then set variables.
MACHINE=$(detect_machine)

set_machine_variables "$MACHINE"

set_tsn_configurations_variables

set_vlan_config

set_interface_low_latency_settings

setup_tx_scheduling_and_rx_classification_settings

isolate_tsn_app_cpu_core

set_napi_and_irqs_priority_and_affinities

load_xdp_program
