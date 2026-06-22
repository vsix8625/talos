#!/usr/bin/env bash
set -e

if [ "$EUID" -ne 0 ]; then
  echo "Error: Run this script with sudo: sudo ./install.sh"
  exit 1
fi

echo "========================================="
echo "   Talos System Monitor - Installer      "
echo "========================================="

FAN_SOURCE="crater/talos_fanctl/release/bin/talos_fanctl"
FAN_DEST="/usr/local/bin/talos_fanctl"
FAN_POLICY="/usr/share/polkit-1/actions/org.talos.pkexec.fancontrol.policy"

POWER_SOURCE="crater/talos_power/release/bin/talos_power"
POWER_DEST="/usr/local/bin/talos_power"
POWER_POLICY="/usr/share/polkit-1/actions/org.talos.pkexec.power.policy"

if [ ! -f "$FAN_SOURCE" ]; then
  echo "Error: Compiled binary not found at $FAN_SOURCE"
  echo "Did you build with 'sk' first?"
  exit 1
fi

if [ ! -f "$POWER_SOURCE" ]; then
  echo "Error: Compiled binary not found at $POWER_SOURCE"
  echo "Did you build with 'sk' first?"
  exit 1
fi

echo "-> Installing talos_fanctl helper to $FAN_DEST..."
cp "$FAN_SOURCE" "$FAN_DEST"
chmod 755 "$FAN_DEST"
chown root:root "$FAN_DEST"

echo "-> Installing talos_power helper to $POWER_DEST..."
cp "$POWER_SOURCE" "$POWER_DEST"
chmod 755 "$POWER_DEST"
chown root:root "$POWER_DEST"

echo "-> Deploying Polkit security policies..."

cat <<EOF > "$FAN_POLICY"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE policyconfig PUBLIC "-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN"
 "https://www.freedesktop.org/standards/PolicyKit/1/policyconfig.dtd">
<policyconfig>
        <vendor>Talos Project</vendor>
        <vendor_url>https://github.com/vsix8625/talos.git</vendor_url>
        <action id="org.talos.pkexec.fancontrol">
                <description>Allow Talos to change fan profiles</description>
                <message>Authentication is required for Talos to adjust system fan profiles.</message>
                <defaults>
                        <allow_any>no</allow_any>
                        <allow_inactive>no</allow_inactive>
                        <allow_active>yes</allow_active> 
                </defaults>
                <annotate key="org.freedesktop.policykit.exec.path">$FAN_DEST</annotate>
        </action>
</policyconfig>
EOF

chmod 644 "$FAN_POLICY"
chown root:root "$FAN_POLICY"

cat <<EOF > "$POWER_POLICY"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE policyconfig PUBLIC "-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN"
 "https://www.freedesktop.org/standards/PolicyKit/1/policyconfig.dtd">
<policyconfig>
        <vendor>Talos Project</vendor>
        <vendor_url>https://github.com/vsix8625/talos.git</vendor_url>
        <action id="org.talos.pkexec.power">
                <description>Allow Talos to shutdown or reboot the machine</description>
                <message>Authentication is required for Talos to alter machine power states.</message>
                <defaults>
                        <allow_any>no</allow_any>
                        <allow_inactive>no</allow_inactive>
                        <allow_active>yes</allow_active> 
                </defaults>
                <annotate key="org.freedesktop.policykit.exec.path">$POWER_DEST</annotate>
        </action>
</policyconfig>
EOF

chmod 644 "$POWER_POLICY"
chown root:root "$POWER_POLICY"

echo "========================================="
echo " Installation successful!"
echo "========================================="
