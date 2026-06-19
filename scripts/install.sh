#!/usr/bin/env bash
set -e

if [ "$EUID" -ne 0 ]; then
  echo "Error: Run this script with sudo: sudo ./install.sh"
  exit 1
fi

echo "========================================="
echo "   Talos System Monitor - Installer      "
echo "========================================="

HELPER_SOURCE="crater/talos_fanctl/release/bin/talos_fanctl"
HELPER_DEST="/usr/local/bin/talos_fanctl"
POLICY_DEST="/usr/share/polkit-1/actions/org.talos.pkexec.fancontrol.policy"

if [ ! -f "$HELPER_SOURCE" ]; then
  echo "Error: Compiled binary not found at $HELPER_SOURCE"
  echo "Did you build with 'sk' first?"
  exit 1
fi

echo "-> Installing talos_fanctl helper to $HELPER_DEST..."
cp "$HELPER_SOURCE" "$HELPER_DEST"
chmod 755 "$HELPER_DEST"
chown root:root "$HELPER_DEST"

echo "-> Deploying Polkit security policy..."

cat <<EOF > "$POLICY_DEST"
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
                <annotate key="org.freedesktop.policykit.exec.path">$HELPER_DEST</annotate>
        </action>
</policyconfig>
EOF

chmod 644 "$POLICY_DEST"
chown root:root "$POLICY_DEST"

echo "========================================="
echo " Installation successful!"
echo "========================================="
