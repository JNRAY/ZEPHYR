# LSB Server

_Page describing integration of so-called "LSB server", a solution designed to collect data from air quality sensors based on Sensiron SEN55 and SCD30 sensors driven by an ESP32 SOC_


# Foreword

## Open-source Software

In order to properly collect and expose data in the contxt for which this project has been set up, we intend to use only open-source and free-to-use software in order to keep costs low and show transparency to those that will provide and use the data we'll be providing.

Additionally, we don't intend to infringe any copyright or proprietary usage limitation by editors of software we choosed to rely on. We think that we're respecting this moto, but as there are many software and licenses involved here: this a complex situation. If you notice any chance that we're not respecting any of those: please notify us so can investigate and remediate...

## Prerequisites

Below are the assumptions that are made in order to properly host the software that are used in this context.

- You dispose of a server with **docker** and **docker compose** plugin installed.
- You dispose of a way to expose the server on the internet: either a hosted service or a broadband.
- You have a domain, prerefarably with a wildcard DNS resolution for any subdomains, to properly host web interfaces that will be built & used.
- You have a letsencrypt account in order to create SSL/TLS certificates used by those interfaces

# Technical documentation

We tried to stay as "standard" as possible, using official releases/images whenever possible and avoided customizing too much the software we integrated so this can be repeated and adapted by other willing to build analog solutions. Don't hesitate to refer to each used software documentation to understand and/or customize configuration to fit your needs and situation.

In the files provided in this repository, mostly docker-compose.yml file, one should consider replacing keywords between `<` and `>` by assets suited to its specific situation or configuration: those can be domain names, IPs, passwords, ...  
=> As a consequence, you'll have to edit/review all files when cloning this repository: comments embedded in files will help filling those values and provide guidance.

We prefered usage of publicly available **docker** images in order to allow better reproductibility of what we've done, so anyone can benefit from the work and design proposed here.  
**Docker** usage also allow for ease of use, configuration and backup of the solution.

## Installation

Please read through the whole **installation** chapter before jumping to **usage** and/or ask questions: many integration details are documented here.

### Clone git repo

As **root** user, if not already done, create a **docker** user, preferably with gid/uid = 1000 and clone this repo as this user's home folder:
```
useradd -u 1000 -g 1000 -b /home/docker -M docker
cd /home
git clone <REPO_URL>
mv REPO_NAME/docker ./
chown -R 1000:1000 ./docker
chown -R 472 ./docker/grafana

```
Now, just go trough all files, looking for keywords as mentioned earlier in order to put your values in place of those so you'll have usable configurations.

=> Below, you'll find a list of all keywords that would need to be replaced once repo is cloned:

| Keyword | File | Usage |  
|---------|------|-------|
| YOUR_DOMAIN | docker/traefik/docker-compose.yml | The domain name that will be used to host solution |
| YOUR_LETENCRYPT_USER_EMAIL | docker/traefik/config/traefik.yml | Email of owner that is used for Letsencrypt SSL certificate |
| TRAEFIK_USER | docker/traefik/config/dynamic/middlewares.yaml | User to connect to Traefik web ui |
| TRAEFIK_PASSWORD | docker/traefik/config/dynamic/middlewares.yaml | Password for above user |
| TRAEFIK_PASSWORD | docker/traefik/config/dynamic/middlewares.yaml | Password hash of above password |
| YOUR_TRUSTED_IP | docker/traefik/config/dynamic/middlewares.yaml | Public IP that you trust (ex: admins of the solution, so they can connect though Traefik filtering |
| YOUR_DOMAIN | docker/wireguard/docker-compose.yml | The domain name that will be used to host solution |
| YOUR_PUBLIC_FQDN | docker/wireguard/docker-compose.yml | FQDN that will be used for VPN clients, likely `vpn.<YOUR_DOMAIN>` |
| WIREGUARD_BCRYPT_PASSWORD_HASH | docker/wireguard/docker-compose.yml | Hash of the password used to acces Wireguard admin web UI. Beware of carefully reading editor documentation that comment in the file points you to for hash generation...|
| PRIVATE_IP_OF_SERVER | docker/wireguard/docker-compose.yml | Private IP of system that will be used to host solution |
| MQTT_USER | docker/mqtt/mosquitto/config/mosquitto.conf | Connection user for MQTT clients |
| YOUR_DOMAIN | docker/mqtt/telegraf-sensor/conf/telegraf.conf | The domain name that will be used to host solution |
| INFLUXDB_ORGANIZATION_NAME | docker/mqtt/telegraf-sensor/conf/telegraf.conf | Organization that you created on InfluxDB admin web UI |
| INFLUXDB_BUCKET_NAME | docker/mqtt/telegraf-sensor/conf/telegraf.conf | Name of bucket that you created on InfluxDB admin web UI |
| INFLUXDB_TOKEN_VALUE | docker/mqtt/telegraf-sensor/conf/telegraf.conf | Value of token for above bucket that you created on InfluxDB admin web UI |
| SENSORS_MQTT_TOPIC | docker/mqtt/telegraf-sensor/conf/telegraf.conf | Value of topic you choosed for sensors to send their metrics |
| MQTT_USER | docker/mqtt/telegraf-sensor/conf/telegraf.conf | User that Telegraf will use to get values from MQTT, that you created earlier in the installation process |
| MQTT_PASSWORD | docker/mqtt/telegraf-sensor/conf/telegraf.conf | Password corresponding to aboce user |
| YOUR_DOMAIN | docker/influxdb/docker-compose.yml | The domain name that will be used to host solution |
| YOUR_DOMAIN | docker/grafana/docker-compose.yml | The domain name that will be used to host solution |
| SMTP_SERVER_IP | docker/grafana/docker-compose.yml | IP of SMTP server to allow Grafana to send emails (optional) |
| SMTP_SERVER_PORT | docker/grafana/docker-compose.yml | Port of SMTP server to allow Grafana to send emails (optional) |
| SMTP_SENDER_EMAIL | docker/grafana/docker-compose.yml | Sender adress used by Grafana to send emails (optional) |
| GRAFANA_ORGANIZATION_NAME | docker/grafana/docker-compose.yml | Organization (you created in Grafana web UI) to allow to publicly expose dashboards without users authentication (optional) |
| PUT_DASHBOARD_DESCRIPTION_HERE | docker/grafana/sen55-graphs.json | Description of the dashboard for users to understand what's in the dashboard |
| SENSOR_DATASOURCE_ID | docker/grafana/sen55-graphs.json | ID of datasource you creaated in Grafana web UI, pointing to InfluxDB bucket for sensors collected data |
| SENSOR_MQTT_TOPIC | docker/grafana/sen55-graphs.json | MQTT topic used by sensors to send data to MQTT |
| PUT_DASHBOARD_DESCRIPTION_HERE | docker/grafana/scd30-graphs.json | Description of the dashboard for users to understand what's in the dashboard |
| SENSOR_DATASOURCE_ID | docker/grafana/scd30-graphs.json | ID of datasource you creaated in Grafana web UI, pointing to InfluxDB bucket for sensors collected data |
| SENSOR_MQTT_TOPIC | docker/grafana/scd30-graphs.json | MQTT topic used by sensors to send data to MQTT |

### Docker configuration

To avoid IP conflict with local network and fix some enrtypoints, we fixed the IP used bu docker, especially the **docker0** bridge interface.  
=> This is done by configuring it in `/etc/docker/daemon.json` fille: refer to the `daemon.json` file in the root of this repository.

### Folder structure

For hosting containers, we choosed to add a **docker** user to the system (user without any admin privileges with uid/gid=1000), but any user will do.  
=> As a consequence, the `docker` folder in this repo is actually the `/home/docker` folder on the server: you may need to adapt volumes path in the compose files to your situation. But, the easier would be to stick to this structure, so you'll have minimal changes to carry out in compose files.

### Traefik

To allow quick and easy configuration and hosting of web interfaces, we rely on the **traefik** container which is a reverse proxy able to generate SSL certificates from **Letencrypt** for each subdomain that will be used in other containers.  

Port 80, should be exposed to the internet so **traefik** can automatically generate SSL certificates through **Letencrypt** certificate athority.  
Port 443 is only needed if you're willing to expose the WebUIs to the internet: but beware of using proper filtering and secure passwords on your interfaces if you're hoing that way !

### Wireguard

A Wireguard VPN will be used so sensors can connect to server to push air-quality measures to server: we choose to use the **wgeasy** container to allow easy wireguard integration/configuration.  

Although it is a docker container, and on CentOS at least, this needs a bit of system tuning on server by adding the `ip_tables` module to kernel at boot time: you should add commands to your `/etc/rc.local` file: see the `rc.local` file in the `docker/wireguard` folder of this repo (or possibly create a file in the `/etc/modprobe.d/` to have this module probed at start-up).  
We prefered the `rc.local` solution as we've added a restart for **mosquitto** container as it does not properly starts on every reboot the server may experience.

### InfluxDB

In order to store measures sent by sensors, we need a DB able to handle so called "time-series": we choosed **InfluxDB**.  
When running container for first time, you'll have to create via web UI:

* an organization
* a bucket (= a DB) for sensor data
* optional: a bucket for weather data, if you choose to collect weather from external sources
* a token with write permission to each bucket for Telegraf (to be used in **mqtt** docker stack)
* a token with read permission to each bucket for Grafana (to be used in **grafana** docker stack)

### MQTT

Sensors send data using MQTT protocol to the server. For the MQTT stack, we choose to use **mosquitto** container as a MQTT broker and **telegraf** as client to get MQTT messages.

A single docker compose file for both container is in the `docker/mqtt` folder of this repo.  
It also contains additional telegraf container for collecting weather data from a public station so we can display wind direction and speed on a map with sensors (see the **grafana** stack).

### Grafana

To display sensors data in bashboards, **Grafana** is a perfect tool.  
Container use a specific uid (472) when running, so don't omit to `chown -R 472 /home/docker/grafana` after cloning this repo.

We have set up dashboards showing graphs and maps with sensors:

* create datasources in Grafana web UI corresponding to each InfluxDB bucket and note their ID aside. When adding datasources, you'll need to use bucket tokens created earlier and user HTTP header authentication. Refer to [Grafana documentation about InfluxDB datasource](https://grafana.com/docs/grafana/latest/datasources/influxdb/configure-influxdb-data-source/#flux-specific-configuration-section)
* refer to JSON files in `docker/rafana` folder of this repo: those have to be imported in the web UI, then adapted to you situation (datasource ID, locations, sensor names, ...).

## Usage

### Accesses
Once installation is properly done, you should consider below recapitulative information to properly  use the solution:

<u>Ports to be publicly exposed:</u>

| Port | Protocol | Usage |
|------|----------|-------|
| 80 | TCP | For Traefik to be able to generate SSL certificates |
| 443 | TCP | Optional: to publicly expose web UI, if needed |
| 51820 | UDP | For sensors to extablish Wireguard VPN connection |

<u>Available web UI URLs:</u>

| URL | Usage |
|-----|-------|
| `https://air-quality.<YOUR_DOMAIN>` | Grafana web UI, used for administration and exposing dashboards |
| `https://influxdb.<YOUR_DOMAIN>` | InfluxDB admin web UI and API used as Grafana datasource |
| `https://traefik.<YOUR_DOMAIN>` | Traefik dashboard, to view & check reverse proxy configuration |
| `https://vpn.<YOUR_DOMAIN>` | Wireguard admin web UI used to create & view VPN accounts for sensors. It is also the VPN server address to be used in the ESP32 code |

### VPN keys

First thing is to create a VPN account for a sensor (one account for each sensor) on the Wireguard web UI.  
As ESP32 library in the code from [ESP32 code repo](https://repo.url) does not take into account pre-shared key, they'll have to be removed from configuration that the webmin created.
To achieve this, stop the wireguard container and edit the `docker/wireguard/data/wg0.json` file and remove the value for `preSharedKey` for each account you created but keep the key in the file or another one will be created, as shown in below snippet example:

```json
(...)
"preSharedKey": "",
(...)
```

### Configure sensor

Refer to [ESP32 code repo](https://repo.url) and include VPN keys in header file before compilation and upload firware to ESP32.  
When sensor is restarted and configured to connect to a Wifi network, you should be able to see it connecting at MQTT level by looking to **mosquitto** container logs.