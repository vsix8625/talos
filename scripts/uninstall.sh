#!/usr/bin/env bash
set -e

if [ "$EUID" -ne 0 ]; then
  echo "Error: Run this script with sudo: sudo ./uninstall.sh"
  exit 1
fi

echo "========================================="
echo " Removing Talos system components...    "
echo "========================================="

echo "-> Removing binaries from /usr/local/bin/..."
rm -f /usr/local/bin/talos_fanctl
rm -f /usr/local/bin/talos_power

echo "-> Purging Polkit policies..."
rm -f /usr/share/polkit-1/actions/org.talos.pkexec.fancontrol.policy
rm -f /usr/share/polkit-1/actions/org.talos.pkexec.power.policy

echo "========================================="
echo " Cleanup complete!"
echo "========================================="
