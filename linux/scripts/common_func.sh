#!/bin/sh

# Check which ps binary is in use (busybox or procps)
# pgrep is not POSIX (not always available) and have a 15 characters limitation
# shellcheck disable=SC2009
ps -V 2>&1 |grep -q procps && PS='ps ax' || PS='ps'
echo "Using ps = ${PS}"

# Detect the machine we are running on.
# Possible echoed values are:
detect_machine ()
{
	if grep -q 'Freescale i\.MX6 ULL 14x14 EVK Board' /sys/devices/soc0/machine; then
		echo 'imx6ullEvkBoard'
	elif grep -q 'Freescale i.MX6 ULL 14x14 RevE EVK Board' /sys/devices/soc0/machine; then
		echo 'imx6ullEvkBoardRevE'
	elif grep -q 'FSL i\.MX8MM.*EVK.*board' /sys/devices/soc0/machine; then
		echo 'imx8mmevk'
	elif grep -q 'NXP i\.MX8MPlus EVK board' /sys/devices/soc0/machine; then
		echo 'imx8mpevk'
	elif grep -q 'Freescale i\.MX8DXL EVK' /sys/devices/soc0/machine; then
		echo 'imx8dxlevk'
	elif grep -q 'NXP i\.MX93 14X14 EVK board' /sys/devices/soc0/machine; then
		echo 'imx93evkauto'
	elif grep -q 'NXP i\.MX93 9x9 Quick Start Board' /sys/devices/soc0/machine; then
		echo 'imx93qsb'
	elif grep -q 'NXP i\.MX93.*EVK.*board' /sys/devices/soc0/machine; then
		echo 'imx93evk'
	elif grep -q 'NXP i\.MX95.*board' /sys/devices/soc0/machine; then
		echo 'imx95evk'
	elif grep -q 'LS1028A RDB Board' /sys/devices/soc0/machine || grep -q 'LS1028ARDB' /etc/hostname; then
		echo 'LS1028A'
	else
		echo 'Unknown'
	fi
}

# Kill process from pid stored in file
# $1: pid file
kill_process_pidfile()
{
	if [ -f "${1}" ]; then
		kill "$(cat "${1}")" > /dev/null 2>&1
		rm "${1}"
	fi
}

# Get value from specific index from a list of space delimited strings
# $1: list of strings with a space delimiter
# $2: 0-based index in the list
get_list_index_val()
{
	# Squeeze all consecutive space occurences
	list="$(echo "$1" | tr -s " ")"

	# cut is fields are 1-based
	index="$2"
	echo "$list" | cut -d" " -f"$((index + 1))"
}

# Get members count of a list of space delimited strings
# $1: list of strings with a space delimiter
get_list_len()
{
	echo "$1" | wc -w
}

# Get one-to-one mapping between tc and hw queues
# $1: number of traffic classes/hw queues
get_one2one_hw_queues_mapping()
{
	num_q="$1"
	hw_queues_mapping="1@0"

	if [ "$num_q" -eq 0 ]; then
		echo ""
	elif [ "$num_q" -eq 1 ]; then
		echo "${hw_queues_mapping}"
	fi

	for q in $(seq 1 "$num_q"); do
		hw_queues_mapping="${hw_queues_mapping} 1@$q"
	done

	echo "${hw_queues_mapping}"
}

# Wait for a file creation with a timeout
# $1: file_name
# $2: timeout value
wait_file_exists()
{
	timeout_cnt="$2"

	while [ ! -f "$1" ]; do
		if [ "$timeout_cnt" -eq 0 ]; then
			echo "error: timeout expired on file creation($1)"
			exit 1
		fi

		sleep 1

		timeout_cnt=$((timeout_cnt-1))
	done
}
