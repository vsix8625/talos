#!/usr/bin/env bash
set -e

if [ "$EUID" -ne 0 ]; then
  echo "Error: Run this script with sudo: sudo ./uninstall.sh"
  exit 1
fi

echo "Removing Talos system components..."

rm -f /usr/local/bin/talos_fanctl
rm -f /usr/share/polkit-1/actions/org.talos.pkexec.fancontrol.policy

echo "Cleanup complete!"
