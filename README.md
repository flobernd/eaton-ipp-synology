# Eaton® Intelligent Power® Protector for Synology® DSM 7

[![MIT License][license-shield]][license-url]
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]

<!-- markdownlint-disable MD033 -->

<br />

<div align="center">
  <a href="https://github.com/flobernd/eaton-ipp-synology">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="images/eaton-logo-dark.png" width="auto" height="80px">
      <source media="(prefers-color-scheme: light)" srcset="images/eaton-logo.png" width="auto" height="80px">
      <img alt="Logo" src="images/eaton-logo.png" width="auto" height="80px">
    </picture>
  </a>
  <h3 align="center">Eaton® Intelligent Power® Protector for Synology® DSM</h3>
  <p align="center">
    Build Eaton® Intelligent Power® Protector for Synology® DiskStation Manager (DSM) 7
    <br />
    <a href="https://www.eaton.com/us/en-us/catalog/backup-power-ups-surge-it-power-distribution/eaton-intelligent-power-protector.html">
      <strong>Explore the official Eaton® IPP documentation</strong>
    </a>
    <br />
    <br />
  </p>
</div>

<!-- markdownlint-enable MD033 -->

## About the Project

![Eaton IPP](./images/ipp.png)

Eaton's Intelligent Power Protector (IPP) software provides graceful, automatic shutdown of network devices during a prolonged power
disruption, preventing data loss and saving work-in-progress.

Eaton IPP is distributed on the offical
[download page](https://www.eaton.com/us/en-us/forms/backup-power-ups-surge-it-distribution/eaton-intelligent-power-protector-software-download-form.html)
for multiple platforms, however - as of today - Eaton does not provide a native DSM package.

Although DSM 7 provides integrated functionality for generic UPS monitoring via NUT, it lacks the necessary configuration options to
precisely control the shutdown time of the NAS in the overall shutdown sequence.

Eaton IPP for DSM connects your protected Synology NAS directly to the UPS Network Management Card, enabling centralized management of the
shutdown sequence. Key benefits include:

- Fine-grained control over the timing of shutdown events
- Flexible configuration of shutdown order and delays across multiple agents
- Assignment of specific power sources or output groups (main, outlet 1, outlet 2)
- Seamless integration with Eaton Intelligent Power Manager (IPM)

With this solution, you can apply consistent shutdown conditions across both IPP agents and IPM-managed servers. For example, you can
ensure the NAS is only powered down after dependent systems, such as servers or clusters, have been safely shut down.

This repository contains tooling to build and package Eaton IPP for Synology DSM 7 using the official Eaton IPP deb installer package.

## Building the Package

There are two primary ways to build this package, depending on your preferred workflow.

### 1. Using the provided Docker Builder Image

A pre-configured Docker builder image is available to streamline the build process. Use the following commands to build the Eaton IPP
package. The Docker builder image is created on demand. Please note that the initial run may take some time, as the Synology Toolkit will
be downloaded during this process:

```bash
git clone https://github.com/flobernd/eaton-ipp-synology

cd build
./build.sh
```

After the build process completes, the final package artifacts will be located in the `build/packages` directory.

> [!NOTE]
> To speed up subsequent builds, both the Docker image and the Docker container are retained.
If you no longer need them, you can remove both manually by running:
>
> ```bash
> docker rm -f dsm-builder-eaton-ipp
> docker rmi -f flobernd/dsm-builder-epyc7002-7.2:latest
> ```

### 2. Manual Build with Synology Developer Toolkit

Alternatively, you can build the package manually using the Synology Developer Toolkit. This approach is ideal if you already have the
toolkit installed or prefer a more hands-on build process.

Detailed setup- and build instructions are available in the official
[Synology Developer Toolkit guide](https://help.synology.com/developer-guide/getting_started/prepare_environment.html).

## Installation

### Configure DSM

It is crucial to disable the native UPS monitoring functionality in DSM by navigating to "Control Panel" > "Hardware & Power" > "UPS" and
ensuring the "Enable UPS support" option is **unchecked**.

<details>
  <summary>Additional context</summary>

  When battery power is low, the native DSM UPS integration will place the NAS into safe mode rather than fully shutting it down. The Eaton
  IPP package replicates this behavior, entering safe mode when its configured power conditions are met.

  Synology has made a - in my opinion - questionable design decision by implementing an automatic reboot mechanism: if the main power
  returns before the UPS is exhausted or its shutdown sequence is completed (i.e., while the NAS is still powered by the UPS), DSM will
  automatically exit safe mode and restart the system.

  While this behavior makes sense in simple scenarios where the UPS is directly attached to the NAS and no other devices are connected
  (or all other devices have a similar recovery functionality), it does not make sense in advanced setups involving network UPS powering
  multiple devices. In this case, the UPS is most of the times configured to keep the shutdown sequence running until the end - even when
  power returns - in order to power cycle all devices (forced reboot).

  If DSM initiates its own reboot before the UPS has completed its shutdown sequence, the NAS becomes susceptible to an improper shutdown
  when power is lost again. This behavior ultimately defeats the purpose of the UPS and undermines the protection it is designed to provide.

  Disabling UPS support is the only way to fully prevent this automatic restart behavior.
</details>

> [!NOTE]
> If the DSM native UPS support is enabled, the package will display a warning at startup.

### Install the Package

Open the "Package Center" and select "Manual Install." Choose the `eaton-ipp-x86_64-*.spk` package file and complete the wizard to install it.

**Important:** Ensure that the "Run after installation" option is unchecked.

### Grant shutdown permissions

DSM 7 runs packages as unprivileged users for security reasons. Shutting down the system is a critical operation that can only be performed
by the `root` user.

The Eaton Intelligent Power Protector for DSM 7 package includes a program that wraps the `synopoweroff -s` system command, which is also
used by the native UPS monitoring to shut down the NAS in case of a low battery event.

To allow Eaton IPP to trigger a safe system shutdown, follow these steps to change the script's owner to `root` and set the `setuid`
permission:

1. If not already enabled, go to "Control Panel" > "Terminal & SNMP" and activate "Enable SSH service".
2. Connect to your NAS via SSH using an admin account:

   ```bash
   ssh -o PubkeyAuthentication=no adminuser@192.168.1.123 # DSM IP address or hostname
   ```

3. Execute the following command and provide your password when prompted:

   ```bash
   sudo /var/packages/eaton-ipp/scripts/update_permissions.sh
   ```

> [!WARNING]
> You must repeat this procedure after reinstalling or upgrading the package, as DSM resets permissions to their defaults.

> [!NOTE]
> If the file permissions are not set correctly, the package will display a warning at startup.

### Start the Package

Navigate to "Package Center" > "Installed" > "Eaton Intelligent Power Protector" and click the "Start" button. If everything is set up
correctly, there should be no warning messages.

### Configure IPP

Access the IPP web interface to begin configuration. The default login credentials are `admin` for both username and password.

After logging in, navigate to the "Auto Discovery" menu to detect and connect to your UPS. Once connected, go to the "Shutdown" menu to
configure the detected power source and specify the corresponding outlet/group.

For more detailed information about configuration options and advanced settings, refer to the
[official Eaton IPP User Guide (PDF)](https://www.eaton.com/content/dam/eaton/products/backup-power-ups-surge-it-power-distribution/power-management-software-connectivity/eaton-intelligent-power-protector/eaton-ipp-user-guide-p-164000291.pdf).

**Important:** In the "Shutdown" > "Configuration" section, ensure that you retain the default settings for both "Shutdown Type" and
"Shutdown Script". Any other value **will not work**!

The default values are:

- "Shutdown Type": `Script`
- "Shutdown Script": `/usr/local/packages/@appstore/eaton-ipp/configs/actions/synopoweroff`

> [!WARNING]
> Changing these options will prevent the shutdown functionality from working properly.

> [!NOTE]
> If the configuration is incorrect, the package will display a warning at startup.

### Test the Shutdown Command

It is highly recommended to verify the shutdown behavior by using the "Test shutdown" option found in the "Shutdown" section of the IPP
web-interface.

If everything is functioning correctly, DSM should enter safe mode after a few seconds, unmounting all volumes and stopping all services.
SSH access will remain available in this state. To exit safe mode, you must either power cycle the device (this is totally safe to do in
this state) or, alternatively, execute `sudo /sbin/reboot` via SSH.

## Future Plans

One thing I would really like to achieve in v2 is to eliminate the need for manual file permission adjustments via SSH. Ideally, all
configuration should be manageable entirely through the DSM web interface.

A promising solution involves leveraging the native DSM UPS support to monitor a "Synology UPS Server" (which is, in reality, a NUT server
under the hood). The package would run its own lightweight NUT server implementation, reporting static UPS values to the NAS
(primarily `ups.status`: `OL` for "online"). IPP would remain configured to execute a script on shutdown, but instead of directly
triggering safe mode, the script would signal the NUT server to change the `ups.status` value to `OB LB` ("on-battery", "low-battery").
This status immediately triggers DSM's built-in safe mode functionality.

Initial proof-of-concept testing has shown this approach to work quite well. Unfortunately, Synology enforces strict validation and does
not allow `127.0.0.1` to be used as the "Synology UPS Server" IP address. However, you can still use the external IP of any NIC, which will
be redirected to `localhost` by the network stack. This does require at least one NIC to be configured with a static IP, but that should be
the case for the vast majority of setups.

Another benefit is that this approach prevents DSM from monitoring a UPS through the native integration while the IPP package is in use.
This avoids the unwanted side effect of DSM automatically rebooting when main power returns. Instead, the internal NUT server continues to
signal `OB LB` until it is explicitly restarted (such as after the next reboot), ensuring that DSM remains in safe mode as intended until
the UPS is exhausted or force-rebooted.

## License

Docker Eaton Intelligent Power Protector for Synology DSM is licensed under the MIT license.

Eaton® Intelligent Power® Protector is licensed under its respective license.

[contributors-shield]: https://img.shields.io/github/contributors/flobernd/eaton-ipp-synology.svg?style=flat-square
[contributors-url]: https://github.com/flobernd/eaton-ipp-synology/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/flobernd/eaton-ipp-synology.svg?style=flat-square
[forks-url]: https://github.com/flobernd/eaton-ipp-synology/network/members
[stars-shield]: https://img.shields.io/github/stars/flobernd/eaton-ipp-synology.svg?style=flat-square
[stars-url]: https://github.com/flobernd/eaton-ipp-synology/stargazers
[issues-shield]: https://img.shields.io/github/issues/flobernd/eaton-ipp-synology.svg?style=flat-square
[issues-url]: https://github.com/flobernd/eaton-ipp-synology/issues
[license-shield]: https://img.shields.io/github/license/flobernd/eaton-ipp-synology.svg?style=flat-square
[license-url]: https://github.com/flobernd/eaton-ipp-synology/blob/master/LICENSE.txt
