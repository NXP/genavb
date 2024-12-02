#! /bin/sh

AVB_APP="avb"
AVB_LOG_FILE=/var/log/$AVB_APP
AVB_PID_FILE=/var/run/$AVB_APP.pid
AVB_TSN_MEDIA_APP_PID_FILE=/var/run/avb_tsn_media_app.pid
AVB_CONTROLLER_PID_FILE=/var/run/avb_avdecc_controller.pid
AVB_CONTROLS_APP_PID_FILE=/var/run/avb_controls_app.pid
AVB_APP_HYBRID_OPTS=""

. "/etc/genavb/config_common"

# Source common genavb functions
. "/etc/genavb/common_func.sh"

. "/etc/genavb/config"

. "$GENAVB_TSN_CFG_FILE"

if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
	. "$APPS_CFG_FILE"
fi

# Parameters:
# $1: sound card name
# $2: direction (playback or capture)
get_sound_card_number()
{
	if [ "$2" = "capture" ]; then
		_alsa_tool="arecord"
	else
		_alsa_tool="aplay"
	fi

	_snd_soc_card_num=$("$_alsa_tool" -l | grep -F "$1" | head -n1 | sed -n 's/.*card \([0-9]*\):.*/\1/p')

	# If not able to get the sound card number for the specific name, use sound card 0
	if [ -z "$_snd_soc_card_num" ]; then
		_snd_soc_card_num="0"
	fi

	echo $_snd_soc_card_num
}

# Parameters:
# $1: configured alsa device
# $2: sound card name
# $3: direction (playback or capture)
get_alsa_device()
{
	_alsa_device=$1

	if [ -z "$1" ] || [ "$1" = "default" ]; then
		_snd_soc_card_num=$(get_sound_card_number "$2" "$3")
		_alsa_device="plughw:$_snd_soc_card_num,0"
	fi

	echo "$_alsa_device"
}

set_machine_variables()
{
	# Some platform-dependant variables.
	case $1 in
	imx6ullEvkBoard*)
		# iMX.6 UltraLiteLite EVK Board
		IRQ_DMA_NAME="dma-controller"
		IRQ_TIMER_NAME="epit"
		IRQ_ETH_NAME="ethernet"
		CFG_EXTERNAL_CONTROLS_APP_OPT="-m Headphone"
		CPU_FREQ="528000"
		if [ "$MACHINE" = "imx6ullEvkBoardRevE" ]; then
			SND_SOC_CARD_NAME="wm8962-audio"
		else
			SND_SOC_CARD_NAME="wm8960-audio"
		fi
		;;
	'imx8mmevk')
		# NXP i.MX8MM EVK Board
		IRQ_DMA_NAME="dma-controller"
		IRQ_TIMER_NAME="gpt"
		IRQ_ETH_NAME="ethernet"
		SND_SOC_CARD_NAME="wm8524-audio"
		# This board does not have an alsa control.
		# Pass dummy as alsa control to make the controls app open the controlled socket anyway
		CFG_EXTERNAL_CONTROLS_APP_OPT="-m dummy"
		# Set Primary video device
		if [ -z "$CFG_PRIMARY_VIDEO_DEVICE" ] || [ "$CFG_PRIMARY_VIDEO_DEVICE" = "default" ]; then
			CFG_PRIMARY_VIDEO_DEVICE="hdmi"
		fi
		# Set MJPEG minimum latency
		if [ -z "$CFG_TOTAL_MJPEG_LATENCY" ] || [ "$CFG_TOTAL_MJPEG_LATENCY" = "default" ]; then
			CFG_TOTAL_MJPEG_LATENCY="74000000"
		fi
		CPU_FREQ="1800000"
		;;
	'imx8mpevk')
		# NXP i.MX8MPlus EVK Board
		if [ "$AVB_MODE" -eq 1 ]; then
			IRQ_DMA_NAME="dma-controller"
			IRQ_TIMER_NAME="302d.*timer"
			IRQ_ETH_NAME="ethernet"
			SND_SOC_CARD_NAME="wm8960-audio"
			CFG_EXTERNAL_CONTROLS_APP_OPT="-m Headphone"
			# Set Primary video device
			if [ -z "$CFG_PRIMARY_VIDEO_DEVICE" ] || [ "$CFG_PRIMARY_VIDEO_DEVICE" = "default" ]; then
				CFG_PRIMARY_VIDEO_DEVICE="hdmi"
			fi
			# Set MJPEG minimum latency
			if [ -z "$CFG_TOTAL_MJPEG_LATENCY" ] || [ "$CFG_TOTAL_MJPEG_LATENCY" = "default" ]; then
				CFG_TOTAL_MJPEG_LATENCY="74000000"
			fi
		else
			ITF=eth1
			IRQ_ETH_NAME="eth1"
		fi
		CPU_FREQ="1800000"
		;;
	'imx8dxlevk')
		# NXP i.MX8DXL EVK Board
		if [ "$AVB_MODE" -eq 1 ]; then
			IRQ_DMA_NAME="edma"
			IRQ_TIMER_NAME="gpt"
			IRQ_ETH_NAME="ethernet"
			SND_SOC_CARD_NAME="wm8960-audio"
			CFG_EXTERNAL_CONTROLS_APP_OPT="-m Headphone"
		else
			ITF=eth0
			IRQ_ETH_NAME="eth0"
		fi
		CPU_FREQ="1200000"
		;;
	'imx93evkauto')
		# NXP i.MX93 14x14 EVK Board
		if [ "$AVB_MODE" -eq 1 ]; then
			IRQ_DMA_NAME="edma2"
			IRQ_TIMER_NAME="pwm"
			IRQ_ETH_NAME="ethernet"

			if [ "$PACKAGE" = "HYBRID" ]; then
				# AVB Hybrid mode with external speaker (MQS)
				SND_SOC_CARD_NAME="mqs-audio"

				# Using external link to connect endpoint (ENET2) to bridge (ENET1):
				# Use hybrid mode with gPTP sync
				AVB_APP_HYBRID_OPTS="-H 2"
			else
				# AVB Endpoint mode with audio hat (cs42448)
				SND_SOC_CARD_NAME="imx-cs42448"
			fi

			CFG_EXTERNAL_CONTROLS_APP_OPT="-m dummy"
		else
			ITF=eth1
			IRQ_ETH_NAME="eth1"
		fi
		;;
	'imx93evk')
		# NXP i.MX93 EVK Board
		if [ "$AVB_MODE" -eq 1 ]; then
			IRQ_DMA_NAME="edma2"
			IRQ_TIMER_NAME="pwm"
			IRQ_ETH_NAME="ethernet"
			SND_SOC_CARD_NAME="wm8962-audio"
			CFG_EXTERNAL_CONTROLS_APP_OPT="-m Headphone"
		else
			ITF=eth1
			IRQ_ETH_NAME="eth1"
		fi
		;;
	'imx93qsb')
		ITF=eth0
		IRQ_ETH_NAME="eth0"
		;;
	'LS1028A')
		ITF=eno2
		;;
	'imx95evk')
		ITF=eth0
		IRQ_ETH_NAME="eth0"
		CPU_FREQ=1800000
		;;
	*)
		IRQ_DMA_NAME="sdma"
		IRQ_TIMER_NAME="gpt"
		IRQ_ETH_NAME="ethernet"
		CFG_EXTERNAL_CONTROLS_APP_OPT="-m dummy"
		;;
	esac

	if [ -n "$SND_SOC_CARD_NAME" ]; then
		CFG_ALSA_PLAYBACK_DEVICE=$(get_alsa_device "$CFG_ALSA_CAPTURE_DEVICE" "$SND_SOC_CARD_NAME" "playback")
		CFG_ALSA_CAPTURE_DEVICE=$(get_alsa_device "$CFG_ALSA_PLAYBACK_DEVICE" "$SND_SOC_CARD_NAME" "capture")
		CFG_EXTERNAL_CONTROLS_APP_OPT="$CFG_EXTERNAL_CONTROLS_APP_OPT -c hw:$(get_sound_card_number "$SND_SOC_CARD_NAME" "playback")"
	fi
}

check_media_clock_config()
{
	# Try to open the mclk_rec_0 in a subshell (no need to explicitly close the fd later) and record
	# the output to analyse if the MCR device exists or not.
	_output_open_fd=$(eval 'command exec {mclk_rec_fd}< /dev/mclk_rec_0' 2>&1)

	if echo "$_output_open_fd" | grep -q "No such device"; then
		echo "Device do not have MCR capabilities"
		# If media application is asking for clock slaving (static CRF listener stream) without a supported MCR device, override
		# it and put as master clock with different streamID than the default to avoid SRP conflicts
		if echo "$CFG_MEDIA_CLOCK" | grep -q -- "-L[[:space:]]\+-S[[:space:]]\+[0-9]\+[[:space:]]\+-c[[:space:]]\+0"; then
			echo "Device can not be a media clock slave, override it to master media clock"

			_static_stream_index=$(echo "$CFG_MEDIA_CLOCK" | grep -oP '(?<=-S )\d+')
			if [ -z "$_static_stream_index" ]; then
				_static_stream_index="0"
			fi

			CFG_MEDIA_CLOCK="$(echo "$CFG_MEDIA_CLOCK -I 0x0000aabbccddeefe" | sed 's/-L/-T/g')"
		fi
	fi
}

print_variables()
{
	echo "GENAVB_CFG_FILE               " "$GENAVB_CFG_FILE"
	echo "APPS_CFG_FILE                 " "$APPS_CFG_FILE"
	echo "CFG_USE_EXTERNAL_CONTROLS     " "$CFG_USE_EXTERNAL_CONTROLS"
	echo "CFG_EXTERNAL_CONTROLS_APP     " "$CFG_EXTERNAL_CONTROLS_APP"
	echo "CFG_EXTERNAL_CONTROLS_APP_OPT " "$CFG_EXTERNAL_CONTROLS_APP_OPT"
	echo "CFG_EXTERNAL_MEDIA_APP        " "$CFG_EXTERNAL_MEDIA_APP"
	echo "CFG_EXTERNAL_MEDIA_APP_OPT    " "$CFG_EXTERNAL_MEDIA_APP_OPT"
}

gst_plugin_setup()
{
	echo "Generating gst plugins registry file"

	if [ -f "$GST_INSPECT1" ]; then
		$GST_INSPECT1 > /dev/null
	fi

	export GST_REGISTRY_UPDATE="no"
}

gst_env_setup()
{
	if [ -z "$WAYLAND_DISPLAY" ] && [ -e "$XDG_RUNTIME_DIR"/wayland-1 ]; then
		# weston now uses wayland-1 as default socket name
		echo "WAYLAND_DISPLAY not declared, use wayland-1 as display socket name"

		export WAYLAND_DISPLAY=wayland-1
	fi
}

# 'Default' alsa mixer setup.
alsa_mixer_setup()
{
	_snd_soc_loaded=$(aplay -l | grep -F "$SND_SOC_CARD_NAME")
	if [ -z "$_snd_soc_loaded" ]; then
		echo "Sound card ($SND_SOC_CARD_NAME) is not registered"
	else
		snd_soc_card_num=$(get_sound_card_number "$SND_SOC_CARD_NAME" "playback")

		case "$MACHINE" in
		'imx6ullEvkBoard')
			# Playback
			amixer -c "$snd_soc_card_num" -q sset Speaker 150,150
			amixer -c "$snd_soc_card_num" -q sset Headphone 100,100
			# Capture: Enable MAIN MIC Channels
			amixer -c "$snd_soc_card_num" -q cset name='Right Input Mixer Boost Switch' on
			amixer -c "$snd_soc_card_num" -q cset name='Right Boost Mixer RINPUT1 Switch' on
			amixer -c "$snd_soc_card_num" -q cset name='Right Boost Mixer RINPUT2 Switch' on
			amixer -c "$snd_soc_card_num" -q cset name='Right Boost Mixer RINPUT3 Switch' off
			amixer -c "$snd_soc_card_num" -q cset name='ADC PCM Capture Volume' 220
			# Capture: Using MAIN MIC to support stereo
			amixer -c "$snd_soc_card_num" -q cset name='ADC Data Output Select' 2
			;;
		'imx8mmevk')
			#The imx8mqevk and imx8mmevk boards do not have alsa sound controls
			echo "This board: $MACHINE Does not have alsa sound controls for sound card: $SND_SOC_CARD_NAME"
			return
			;;
		'imx8mpevk')
			# Playback
			amixer -c "$snd_soc_card_num" -q sset Speaker 150,150
			amixer -c "$snd_soc_card_num" -q sset Headphone 100,100
			# Capture
			amixer -c "$snd_soc_card_num" -q sset Capture 50,50
			;;
		'imx8dxlevk')
			# Playback
			amixer -c "$snd_soc_card_num" -q sset Speaker 150,150
			amixer -c "$snd_soc_card_num" -q sset Headphone 100,100
			# Capture
			amixer -c "$snd_soc_card_num" -q sset Capture 50,50
			;;
		'imx93evkauto')
			if [ "$PACKAGE" = "ENDPOINT" ] && [ "$AVB_MODE" -eq 1 ]; then
				# Playback
				amixer -c "$snd_soc_card_num" -q sset DAC1 230,230
				amixer -c "$snd_soc_card_num" -q sset DAC2 230,230
				amixer -c "$snd_soc_card_num" -q sset DAC3 230,230
				amixer -c "$snd_soc_card_num" -q sset DAC4 230,230
			fi
			;;
		'imx93evk'|'imx6ullEvkBoardRevE')
			# Playback
			amixer -c "$snd_soc_card_num" -q sset Speaker 90,90
			amixer -c "$snd_soc_card_num" -q sset Headphone 90,90
			# Capture
			amixer -c "$snd_soc_card_num" -q sset Capture 50,50
			;;
		*)
			echo "Unknown machine: can not setup alsa control"
			return
			;;
		esac

		echo "Alsa mixer setup done for $SND_SOC_CARD_NAME"
	fi
}

stop_tsn_stack()
{
	tsn.sh stop
}

start_tsn_stack()
{
	tsn.sh start
}

avdecc_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_avdecc_media_stack_rx
	rm -fr /dev/ipc_avdecc_media_stack_tx
	rm -fr /dev/ipc_media_stack_avdecc_rx
	rm -fr /dev/ipc_media_stack_avdecc_tx

	rm -fr /dev/ipc_media_stack_maap_rx
	rm -fr /dev/ipc_media_stack_maap_tx
	rm -fr /dev/ipc_maap_media_stack_rx
	rm -fr /dev/ipc_maap_media_stack_tx
	rm -fr /dev/ipc_maap_media_stack_sync_rx
	rm -fr /dev/ipc_maap_media_stack_sync_tx

	rm -fr /dev/ipc_avdecc_controlled_rx
	rm -fr /dev/ipc_avdecc_controlled_tx
	rm -fr /dev/ipc_controlled_avdecc_rx
	rm -fr /dev/ipc_controlled_avdecc_tx

	rm -fr /dev/ipc_avdecc_controller_rx
	rm -fr /dev/ipc_avdecc_controller_tx
	rm -fr /dev/ipc_controller_avdecc_rx
	rm -fr /dev/ipc_controller_avdecc_tx
	rm -fr /dev/ipc_avdecc_controller_sync_rx
	rm -fr /dev/ipc_avdecc_controller_sync_tx

	mknod /dev/ipc_avdecc_media_stack_rx c "$major" 2
	mknod /dev/ipc_avdecc_media_stack_tx c "$major" 3
	mknod /dev/ipc_media_stack_avdecc_rx c "$major" 4
	mknod /dev/ipc_media_stack_avdecc_tx c "$major" 5

	mknod /dev/ipc_media_stack_maap_rx c "$major" 6
	mknod /dev/ipc_media_stack_maap_tx c "$major" 7
	mknod /dev/ipc_maap_media_stack_rx c "$major" 108
	mknod /dev/ipc_maap_media_stack_tx c "$major" 109
	mknod /dev/ipc_maap_media_stack_sync_rx c "$major" 110
	mknod /dev/ipc_maap_media_stack_sync_tx c "$major" 111

	mknod /dev/ipc_avdecc_controlled_rx c "$major" 102
	mknod /dev/ipc_avdecc_controlled_tx c "$major" 103
	mknod /dev/ipc_controlled_avdecc_rx c "$major" 10
	mknod /dev/ipc_controlled_avdecc_tx c "$major" 11

	mknod /dev/ipc_avdecc_controller_rx c "$major" 12
	mknod /dev/ipc_avdecc_controller_tx c "$major" 13
	mknod /dev/ipc_controller_avdecc_rx c "$major" 14
	mknod /dev/ipc_controller_avdecc_tx c "$major" 15
	mknod /dev/ipc_avdecc_controller_sync_rx c "$major" 16
	mknod /dev/ipc_avdecc_controller_sync_tx c "$major" 17
}

avtp_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_media_stack_avtp_rx
	rm -fr /dev/ipc_media_stack_avtp_tx
	rm -fr /dev/ipc_avtp_media_stack_rx
	rm -fr /dev/ipc_avtp_media_stack_tx

	rm -fr /dev/ipc_avtp_stats_rx
	rm -fr /dev/ipc_avtp_stats_tx

	rm -fr /dev/ipc_avdecc_avtp_rx
	rm -fr /dev/ipc_avdecc_avtp_tx
	rm -fr /dev/ipc_avtp_avdecc_rx
	rm -fr /dev/ipc_avtp_avdecc_tx

	mknod /dev/ipc_media_stack_avtp_rx c "$major" 22
	mknod /dev/ipc_media_stack_avtp_tx c "$major" 23
	mknod /dev/ipc_avtp_media_stack_rx c "$major" 124
	mknod /dev/ipc_avtp_media_stack_tx c "$major" 125

	mknod /dev/ipc_avtp_stats_rx c "$major" 44
	mknod /dev/ipc_avtp_stats_tx c "$major" 45

	mknod /dev/ipc_avdecc_avtp_rx c "$major" 200
	mknod /dev/ipc_avdecc_avtp_tx c "$major" 201
	mknod /dev/ipc_avtp_avdecc_rx c "$major" 206
	mknod /dev/ipc_avtp_avdecc_tx c "$major" 207
}

clock_domain_ipc_nodes ()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_media_stack_clock_domain_rx
	rm -fr /dev/ipc_media_stack_clock_domain_tx
	rm -fr /dev/ipc_clock_domain_media_stack_rx
	rm -fr /dev/ipc_clock_domain_media_stack_tx
	rm -fr /dev/ipc_clock_domain_media_stack_sync_rx
	rm -fr /dev/ipc_clock_domain_media_stack_sync_tx

	mknod /dev/ipc_media_stack_clock_domain_rx c "$major" 38
	mknod /dev/ipc_media_stack_clock_domain_tx c "$major" 39
	mknod /dev/ipc_clock_domain_media_stack_rx c "$major" 140
	mknod /dev/ipc_clock_domain_media_stack_tx c "$major" 141
	mknod /dev/ipc_clock_domain_media_stack_sync_rx c "$major" 142
	mknod /dev/ipc_clock_domain_media_stack_sync_tx c "$major" 143
}

media_device_nodes()
{
	major=$(grep media_drv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/media_queue_net
	rm -fr /dev/media_queue_api

	mknod /dev/media_queue_net c "$major" 0
	mknod /dev/media_queue_api c "$major" 1
}

mclock_nodes()
{
	major=$(grep mclock /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/mclk_rec_0
	rm -fr /dev/mclk_gen_0
	rm -fr /dev/mclk_ptp_0
	rm -fr /dev/mclk_ptp_1

	mknod /dev/mclk_rec_0 c "$major" 0
	mknod /dev/mclk_gen_0 c "$major" 8
	mknod /dev/mclk_ptp_0 c "$major" 16
	mknod /dev/mclk_ptp_1 c "$major" 17
}

mtimer_nodes()
{
	major=$(grep mtimer /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/mclk_rec_timer_0
	rm -fr /dev/mclk_gen_timer_0
	rm -fr /dev/mclk_ptp_timer_0
	rm -fr /dev/mclk_ptp_timer_1

	mknod /dev/mclk_rec_timer_0 c "$major" 0
	mknod /dev/mclk_gen_timer_0 c "$major" 8
	mknod /dev/mclk_ptp_timer_0 c "$major" 16
	mknod /dev/mclk_ptp_timer_1 c "$major" 17
}

setup_device_nodes()
{
	if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
		# Check that the ipc kernel module is already loaded: tsn starting script should have done that.
		ipc_loaded=$(lsmod |grep genavbtsn_ipc)
		if [ -z "${ipc_loaded}" ]; then
			echo "Failed to create ipc device nodes: genavbtsn_ipc kernel module is not loaded."
			exit 1
		fi

		# Check that net avb kernel module is already loaded: tsn starting script should have done that.
		net_avb_loaded=$(lsmod |grep genavbtsn_net_avb)
		if [ -z "${net_avb_loaded}" ]; then
			echo "Failed to create net avb device nodes: genavbtsn_avb kernel module is not loaded."
			exit 1
		fi

		avdecc_ipc_nodes
		avtp_ipc_nodes
		clock_domain_ipc_nodes
		media_device_nodes
		mclock_nodes
		mtimer_nodes
	fi
}

set_irq_priority_and_affinity()
{
	if [ "$SET_AFFINITY" -eq 0 ]; then
		return
	fi

	# DMA IRQ
	if [ -n "$IRQ_DMA_NAME" ]; then
		irq_nb=$(grep $IRQ_DMA_NAME /proc/interrupts | awk -F: '{ print $1 }')
		if [ -n "$irq_nb" ]; then
			for _irq_nb in $irq_nb; do
				irq_pid=$(pgrep -f "irq/$_irq_nb-")
				if [ -n "$irq_pid" ]; then
					for _irq_pid in $irq_pid; do
						echo "Setting IRQ/$_irq_nb (DMA) priority to $IRQ_PRIO (pid: $_irq_pid), with cpu affinity mask $CPU_MASK"
						chrt -pf "$IRQ_PRIO" "$_irq_pid"
					done
					[ "$NB_CPU" -gt 1 ] && echo "$CPU_MASK" > /proc/irq/"$_irq_nb"/smp_affinity
				else
					echo "!!! WARNING: Setting IRQ/$_irq_nb (DMA) priority to $IRQ_PRIO failed!"
				fi
			done
		else
			echo "!!! WARNING: There is no matching dma interrupt!"
		fi
	fi

	# HW TIMER IRQ
	if [ -n "$IRQ_TIMER_NAME" ]; then
		irq_nb=$(grep $IRQ_TIMER_NAME /proc/interrupts | awk -F: '{ print $1 }')
		irq_nb=${irq_nb#" "}
		if [ -n "$irq_nb" ]; then
			echo "Setting IRQ/$irq_nb (TIMER) cpu affinity mask to $CPU_MASK"
			[ "$NB_CPU" -gt 1 ] && echo "$CPU_MASK" > /proc/irq/"$irq_nb"/smp_affinity
		else
			echo "!!! WARNING: Setting IRQ/$irq_nb (TIMER) cpu affinity mask failed!"
		fi
	fi

	# Genavb timer kernel thread
	kthread_pid=$(pgrep -f 'avb timer')
	if [ -n "$kthread_pid" ]; then
		echo "Setting AVB kthread priority to $KTHREAD_PRIO, with cpu affinity mask $CPU_MASK"
		chrt -pf "$KTHREAD_PRIO" "$kthread_pid"
		[ "$NB_CPU" -gt 1 ] && taskset -p "$CPU_MASK" "$kthread_pid"
	else
		echo "!!! WARNING: Setting AVB kthread priority to $KTHREAD_PRIO failed!"
	fi

	# MCR kernel thread (if used)
	mcr_kthread_pid=$(pgrep -f 'mcr handler')
	if [ -n "$mcr_kthread_pid" ]; then
		echo "Setting MCR kthread priority to $MCR_KTHREAD_PRIO, with cpu affinity mask $CPU_MASK"
		chrt -pf "$MCR_KTHREAD_PRIO" "$mcr_kthread_pid"
		[ "$NB_CPU" -gt 1 ] && taskset -p "$CPU_MASK" "$mcr_kthread_pid"
	fi

	# ETHERNET IRQ(S)
	if [ -n "$IRQ_ETH_NAME" ]; then
		irq_nb=$(grep $IRQ_ETH_NAME /proc/interrupts | awk -F: '{ print $1 }')
		if [ -n "$irq_nb" ]; then
			for _irq_nb in $irq_nb; do
				if [ -n "$_irq_nb" ]; then
					echo "Setting IRQ/$_irq_nb (Ethernet) cpu affinity mask to $CPU_MASK"
					[ "$NB_CPU" -gt 1 ] && echo "$CPU_MASK" > /proc/irq/"$_irq_nb"/smp_affinity
				else
					echo "!!! WARNING: Setting IRQ/$_irq_nb (Ethernet) cpu affinity mask failed!"
				fi
			done
		else
			echo "!!! WARNING: There is no matching ethernet interrupt!"
		fi
	fi
}

set_ksoftirqd_priority()
{
	echo "Setting ksoftirqd/$CPU_CORE priority to SCHED_FIFO 1"
	ksoftirqd_pid=$(pgrep ksoftirqd/"$CPU_CORE")
	chrt -pf 1 "$ksoftirqd_pid"

	echo "Setting ksoftirqd/$MEDIA_APP_CPU_CORE priority to SCHED_FIFO 1"
	ksoftirqd_pid=$(pgrep ksoftirqd/"$MEDIA_APP_CPU_CORE")
	chrt -pf 1 "$ksoftirqd_pid"
}

# Disable/Enable the cpuidle state, for older kernels
# that do not support Sysfs PM QoS
# $1 : cpu core number
# $2 : disable state entry (0 or 1)
set_cpu_idle_state_disable()
{
	if [ ! -d /sys/devices/system/cpu/cpu"$1"/cpuidle ]; then
		echo "CPU#$1 idle entry is not present is sysfs"
		return;
	fi

	# Check how many cpuidle states there are
	NB_STATES=$(find /sys/devices/system/cpu/cpu"$1"/cpuidle/state* -maxdepth 0 | wc -l)
	for i in $(seq 0 $((NB_STATES-1)))
	do
		_latency=$(cat /sys/devices/system/cpu/cpu"$1"/cpuidle/state"$i"/latency)
		# Disable/Enable cpu idle states with latency greater thatn 10us
		if [ "$_latency" -gt 10 ]; then
			echo "Set CPU#$1 idle state$i disable to ($2)"
			echo "$2" > /sys/devices/system/cpu/cpu"$1"/cpuidle/state"$i"/disable
		fi
	done
}

# Set the CPU power management states
# Enable or Disable cpu idle states for CPU cores
# Running AVB Related tasks
# $1 : 1 (enable) or 0 (disable)
set_cpu_power_management()
{
	if [ "$1" -eq 0 ]; then
		# Disable CPU idle deep state transitions exceeding 10us
		qos_resume_latency=10
		disable=1
	else
		qos_resume_latency=0
		disable=0
	fi

	if [ -f "/sys/devices/system/cpu/cpu$CPU_CORE/power/pm_qos_resume_latency_us" ]; then
		echo "$qos_resume_latency" > /sys/devices/system/cpu/cpu"$CPU_CORE"/power/pm_qos_resume_latency_us
	else
		set_cpu_idle_state_disable "$CPU_CORE" $disable
	fi

	if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
		if [ -f "/sys/devices/system/cpu/cpu$MEDIA_APP_CPU_CORE/power/pm_qos_resume_latency_us" ]; then
			echo "$qos_resume_latency" > /sys/devices/system/cpu/cpu"$MEDIA_APP_CPU_CORE"/power/pm_qos_resume_latency_us
		else
			set_cpu_idle_state_disable "$MEDIA_APP_CPU_CORE" $disable
		fi
	fi

}

disable_dynamic_freq_scaling()
{
	case "$MACHINE" in
	'imx93evk')
		# Disable DDR scaling
		echo "0" > /sys/devices/platform/imx93-lpm/auto_clk_gating
		echo "Disable DDR frequency scaling"
		;;
	'imx8mpevk' | 'imx8mmevk')
		# Disable imx busfreq
		echo "0" > /sys/bus/platform/drivers/imx_busfreq/busfreq/enable
		echo "Disable BUS frequency scaling"
		;;
	imx6ullEvkBoard*)
		# Disable imx busfreq
		echo "0" > /sys/bus/platform/drivers/imx_busfreq/soc:busfreq/enable
		echo "Disable BUS frequency scaling"
		;;
	*)
		return
		;;
	esac
}

setup()
{
	# Set userspace governor with the desired CPU frequency
	if [ -n "$CPU_FREQ" ]; then
		echo userspace > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
		echo "$CPU_FREQ" > /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed
	fi

	if [ "$RT_THROTTLING" -eq 1 ]; then
		# Disable real-time throttling
		echo -1 > /proc/sys/kernel/sched_rt_runtime_us
	fi

	# Disable CPU idle deep state transitions exceeding 10us
	set_cpu_power_management 0

	# Disable dynamic frequency scaling for better real time performance
	disable_dynamic_freq_scaling

	if [ "$FORCE_100M" -eq 1 ]; then
		# Advertise 100 Mbps full-duplex
		ethtool -s ${ITF} advertise 0x8
	fi

	if [ "$AVB_MODE" -eq 1 ]; then
		setup_device_nodes

		set_irq_priority_and_affinity

		set_ksoftirqd_priority
	fi
}

stop_genavb()
{
	kill_process_pidfile $AVB_PID_FILE
}

stop_avb_apps()
{
	kill_process_pidfile $AVB_TSN_MEDIA_APP_PID_FILE
	kill_process_pidfile $AVB_CONTROLS_APP_PID_FILE
	kill_process_pidfile $AVB_CONTROLLER_PID_FILE
}

stop_tsn_apps()
{
	kill_process_pidfile $AVB_TSN_MEDIA_APP_PID_FILE
}

stop()
{
	if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
		stop_avb_apps
		stop_tsn_apps
		stop_genavb
	fi

	# Restore CPU idle deep state transitions latency
	set_cpu_power_management 1
}

stop_all()
{
	# stop avb process and application
	stop
	# stop tsn process
	stop_tsn_stack
}

start_genavb()
{
	if [ "$CFG_TSN_MULTI_ENDPOINTS" -eq 1 ]; then
		# shellcheck disable=SC2086 # Intentional splitting of options
		taskset "$CPU_MASK" $AVB_APP -p 2 -f "$GENAVB_CFG_FILE" $AVB_APP_HYBRID_OPTS> $AVB_LOG_FILE 2>&1 &
	else
		# shellcheck disable=SC2086 # Intentional splitting of options
		taskset "$CPU_MASK" $AVB_APP -f "$GENAVB_CFG_FILE" $AVB_APP_HYBRID_OPTS > $AVB_LOG_FILE 2>&1 &
	fi

	echo $! > $AVB_PID_FILE
}

start_avb_apps()
{
	alsa_mixer_setup

	gst_env_setup

	gst_plugin_setup

	if [ "$CFG_USE_EXTERNAL_CONTROLS" -eq 1 ]; then
		# shellcheck disable=SC2086 # Intentional splitting of options
		taskset "$CPU_MASK" "$CFG_EXTERNAL_CONTROLS_APP" $CFG_EXTERNAL_CONTROLS_APP_OPT > /var/log/avb_controls_app 2>&1 &

		echo $! > $AVB_CONTROLS_APP_PID_FILE
	fi

	# Check media clock config and disable slaving if needed
	check_media_clock_config

	# Expand the needed variables (primary video device, clock slaving config ...)
	eval "CFG_EXTERNAL_MEDIA_APP_OPT=\"${CFG_EXTERNAL_MEDIA_APP_OPT}\""

	# shellcheck disable=SC2086 # Intentional splitting of options
	taskset "$MEDIA_APP_CPU_MASK" "$CFG_EXTERNAL_MEDIA_APP" $CFG_EXTERNAL_MEDIA_APP_OPT > /var/log/avb_media_app 2>&1 &

	echo $! > $AVB_TSN_MEDIA_APP_PID_FILE
}

start_tsn_apps()
{

	# shellcheck disable=SC2086 # Intentional splitting of options
	taskset "$MEDIA_APP_CPU_MASK" "$CFG_EXTERNAL_MEDIA_APP" $CFG_EXTERNAL_MEDIA_APP_OPT &

	echo $! > $AVB_TSN_MEDIA_APP_PID_FILE
}

start()
{
	echo "Starting GenAVB/TSN $PACKAGE stack"

	stop

	# Launch the tsn stack script before proceeding to make sure all needed
	# setup (kernel modules insertion, system cfg file handling ...) is done.
	start_tsn_stack

	setup

	if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
		if [ "$AVB_MODE" -eq 1 ]; then
			echo "Starting AVB stack with configuration file: " "$GENAVB_CFG_FILE"

			start_genavb

			sleep 1 # Work-around cases where the AVB stack isn't ready when the controls app (or media app) starts

			echo "Starting AVB demo with profile " "$APPS_CFG_FILE"

			start_avb_apps
		else
			echo "Starting TSN demo with profile " "$APPS_CFG_FILE"
			# TSN app is using cyclic timers based on system clock: Wait some time to give gPTP (and phc2sys) time
			# to synchronize and avoid having big system/gptp time offsets while the app is running
			sleep 2
			start_tsn_apps
		fi
	fi
}

############################ MAIN ############################

PATH=/sbin:/usr/sbin:/bin:/usr/bin:.

GST_INSPECT1="/usr/bin/gst-inspect-1.0"

if [ -z "$AVB_MODE" ]; then
	AVB_MODE=0
fi

NB_CPU=$(grep -c processor /proc/cpuinfo)

if  [ "$NB_CPU" -gt 2 ];then
	CPU_CORE="${AVB_STACK_CORE_GT2C}"
	[ $AVB_MODE -eq 1 ] && MEDIA_APP_CPU_CORE="${AVB_APP_CORE_GT2C}" || MEDIA_APP_CPU_CORE="${TSN_APP_CORE_GT2C}"
elif  [ "$NB_CPU" -eq 2 ];then
	CPU_CORE="${AVB_STACK_CORE_2C}"
	[ $AVB_MODE -eq 1 ] && MEDIA_APP_CPU_CORE="${AVB_APP_CORE_2C}" || MEDIA_APP_CPU_CORE="${TSN_APP_CORE_2C}"
else
	CPU_CORE="${AVB_STACK_CORE_1C}"
	[ $AVB_MODE -eq 1 ] && MEDIA_APP_CPU_CORE="${AVB_APP_CORE_1C}" || MEDIA_APP_CPU_CORE="${TSN_APP_CORE_1C}"
fi

CPU_MASK="$(( 1 << CPU_CORE ))"
MEDIA_APP_CPU_MASK="$(( 1 << MEDIA_APP_CPU_CORE ))"

# Detect the platform we are running on and set $MACHINE accordingly, then set variables.
MACHINE=$(detect_machine)

# default configuration matching most common machines
ITF=eth0
IRQ_PRIO=60
KTHREAD_PRIO=60
MCR_KTHREAD_PRIO=1

if [ "$AVB_MODE" -eq 1 ]; then
	FORCE_100M=1
else
	FORCE_100M=0
fi

if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
	SET_AFFINITY=1
	RT_THROTTLING=1
else
	SET_AFFINITY=0
	RT_THROTTLING=0
fi

set_machine_variables "$MACHINE"

case "$1" in
start)
	#print_variables
	start
	;;
stop)
	stop
	;;
stop_all)
	stop_all
	;;
restart)
	stop ; sleep 1; start
	;;
restart_all)
	stop_all ; sleep 1; start
	;;
*)
	echo "Usage: $0 start|stop|stop_all|restart|restart_all" >&2
	exit 3
	;;
esac
