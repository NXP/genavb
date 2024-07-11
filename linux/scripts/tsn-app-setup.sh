#!/bin/sh

if [ $# -ne 1 ]; then
	echo "$(basename "$0") <interface>"
	exit 1
fi

ITF=$1

ITF_IRQ=$(grep "$ITF" /proc/interrupts | awk -F: '{ print $1 }')

NB_CPU=$(grep -c processor /proc/cpuinfo)
# Should be aligned with the configuration in avb.sh (which launches the tsn-app)
if  [ "$NB_CPU" -gt 2 ];then
	TSN_APP_CPU_MASK=4
	TSN_APP_CPU_CORE=2
else
	TSN_APP_CPU_MASK=2
	TSN_APP_CPU_CORE=1
fi

# Qbv Config
# PCP mapping (802.1Q Table 8-5, no SR Class, 5 traffic classes):
#    * prio {0, 1, 8-15}              -> TC 0
#    * prio {2, 3}                    -> TC 1
#    * prio {4, 5 (tsn app traffic)}  -> TC 2
#    * prio 6 (gptp/srp traffic)      -> TC 3
#    * prio 7                         -> TC 4
NUM_TC=5
NUM_TC_MASK="0x1f"
PCP_TO_QOS_MAP="0 0 1 1 2 2 3 4 0 0 0 0 0 0 0 0"
HW_QUEUES_MAPPING="1@0 1@1 1@2 1@3 1@4" # 1:1 mapping between HW queues and traffic classes
TSN_TRAFFIC_TC=2         # TC 2 for tsn-app traffic
TSN_GATE_MASK=0x$((1 << TSN_TRAFFIC_TC))
NON_TSN_GATE_MASK=$(printf "%x" $((~TSN_GATE_MASK & NUM_TC_MASK)))
TSN_GATE_NS=4000 # gate window in ns

. "/etc/genavb/config_tsn"

if [ ! -e "$APPS_CFG_FILE" ]; then
	echo "TSN app configuration file '$APPS_CFG_FILE' not found"
	exit 1
fi

. "$APPS_CFG_FILE"

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
	echo "# Disable coalescing on TSN queue ($TSN_TRAFFIC_TC) and flow control on interface $ITF"
	# Disable coalescing on TSN queue
	ethtool --per-queue "$ITF" queue_mask "$TSN_GATE_MASK" --coalesce rx-usecs 16 tx-usecs 10000 tx-frames 1
	# Disable flow control
	ethtool -A "$ITF" autoneg off rx off tx off
	# Changing pause parameters would cause link state changes, wait for link up.
	wait_link_up "1" "5"
}

setup_qdiscs_and_filters()
{
	echo "# Setup taprio qdisc on interface $ITF"
	# Setup 802.1Qbv (using tc taprio qdisc).
	tc qdisc del dev "$ITF" root

	# Setup 802.1Qbv (using tc taprio qdisc).
	tc qdisc replace dev "$ITF" root taprio \
		num_tc "${NUM_TC}" \
		map ${PCP_TO_QOS_MAP} \
		queues ${HW_QUEUES_MAPPING} \
		base-time "$QBV_START_PHASE_SHIFT_NS" \
		sched-entry S "$TSN_GATE_MASK" "$TSN_GATE_NS" \
		sched-entry S "$NON_TSN_GATE_MASK" $((PERIOD_NS - TSN_GATE_NS)) \
		flags 0x2

	echo "# Setup RX classification using flower qdisc on interface $ITF"
	# Setup Rx classification (using tc flower qdisc).
	# Must be called after taprio_config (depends on
	# traffic class definition from taprio).
	modprobe cls_flower
	tc qdisc add dev "$ITF" ingress
	tc filter del dev "$ITF" ingress
	# Incoming TSN app traffic goes into same TC (HW queue) as the TX
	tc filter add dev "$ITF" parent ffff: protocol 802.1Q flower vlan_prio 5 hw_tc $TSN_TRAFFIC_TC
	# Incoming Tagged best effort traffic to TC 1
	tc filter add dev "$ITF" parent ffff: protocol 802.1Q flower vlan_prio 0 hw_tc 1

}

isolate_tsn_app_cpu_core()
{
	echo "# Move all processes off CPU core $TSN_APP_CPU_CORE used for tsn-app"

	# Move processes off TSN app core. Ignore errors
	# for unmoveable processes.
	for i in $(ps aux | grep -v PID | grep -v napi/"$ITF" | awk '{print $2;}'); do
		if ! ps -p "$i" > /dev/null; then
			# Not a running process, skip
			continue;
		fi

		curr_affinity="0x$(taskset -p "$i" | cut -d ":" -f2 | tr -d "[:space:]")"
		new_affinity="$(printf "%x" $((curr_affinity & ~TSN_APP_CPU_MASK)))";

		taskset -p "$new_affinity" "$i" > /dev/null 2>&1;
	done

	# Move workqueues off CPU core 2.
	for i in $(find /sys/devices/virtual/workqueue -name cpumask); do echo "$(printf "%x" $((0xf & ~TSN_APP_CPU_MASK)))" > "$i"; done
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

	taskset -p "$best_effort_cpu_mask" "$(pgrep napi/"$ITF"-rx-1)"
	taskset -p "$best_effort_cpu_mask" "$(pgrep napi/"$ITF"-rx-3)"
	taskset -p "$best_effort_cpu_mask" "$(pgrep napi/"$ITF"-rx-4)"
	taskset -p "$best_effort_cpu_mask" "$(pgrep napi/"$ITF"-tx-0)"
	taskset -p "$best_effort_cpu_mask" "$(pgrep napi/"$ITF"-zc-0)"
	taskset -p "$best_effort_cpu_mask" "$(pgrep napi/"$ITF"-zc-1)"
	taskset -p "$best_effort_cpu_mask" "$(pgrep napi/"$ITF"-zc-3)"
	taskset -p "$best_effort_cpu_mask" "$(pgrep napi/"$ITF"-zc-4)"

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
	for irq in $ITF_IRQ; do
		taskset -p "$eth_irq_cpu_mask" "$(pgrep irq/"$irq"-"$ITF")"
		echo "$eth_irq_cpu_mask" > /proc/irq/"$irq"/smp_affinity
		chrt -pf 66 "$(pgrep irq/"$irq"-"$ITF")"
	done

	# gPTP queues on
	if  [ "$NB_CPU" -gt 2 ];then
		# CPU core 1 (2nd core)
		gptp_cpu_mask=2
	else
		# CPU core 0 (1st core)
		gptp_cpu_mask=1
	fi

	taskset -p "$gptp_cpu_mask" "$(pgrep napi/"$ITF"-rx-0)"
	taskset -p "$gptp_cpu_mask" "$(pgrep napi/"$ITF"-tx-3)"
	# real-time prio, lower than gPTP daemon
	chrt -pf 1 "$(pgrep napi/"$ITF"-rx-0)"
	chrt -pf 1 "$(pgrep napi/"$ITF"-tx-3)"

	# TSN traffic queues on same core as the tsn app
	# same prio as tsn-app for AF_XDP queue
	# in AF_PACKET mode, give network queues a
	# higher prio than tsn-app
	taskset -p "$TSN_APP_CPU_MASK" "$(pgrep napi/"$ITF"-zc-2)"
	taskset -p "$TSN_APP_CPU_MASK" "$(pgrep napi/"$ITF"-rx-2)"
	taskset -p "$TSN_APP_CPU_MASK" "$(pgrep napi/"$ITF"-tx-2)"
	chrt -pf 60 "$(pgrep napi/"$ITF"-zc-2)"
	chrt -pf 61 "$(pgrep napi/"$ITF"-rx-2)"
	chrt -pf 61 "$(pgrep napi/"$ITF"-tx-2)"
}

set_vlan_config

set_interface_low_latency_settings

setup_qdiscs_and_filters

isolate_tsn_app_cpu_core

set_napi_and_irqs_priority_and_affinities

load_xdp_program
