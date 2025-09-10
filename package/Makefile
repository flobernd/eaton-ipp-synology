include /env.mak

DOWNLOAD_URL := https://www.eaton.com/content/dam/eaton/products/backup-power-ups-surge-it-power-distribution/power-management-software-connectivity/eaton-intelligent-power-protector/software/ippv1-75/ipp-linux_1.75.183-1_amd64.deb

IPP_PKGNAME := ipp.deb
IPP_DPKG_DIR := ipppkg
IPP_INSTALL_DIR := build

all: $(IPP_INSTALL_DIR) build

$(IPP_PKGNAME):
	curl $(DOWNLOAD_URL) \
		--silent \
		--show-error \
		--insecure \
		--compressed \
		-H 'User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:142.0) Gecko/20100101 Firefox/142.0' \
		-H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8' \
		-H 'Accept-Language: en-US,en;q=0.5' \
		-H 'Accept-Encoding: gzip, deflate, br, zstd' \
		--output ${IPP_PKGNAME}

$(IPP_INSTALL_DIR): $(IPP_PKGNAME)
	dpkg -x $(IPP_PKGNAME) $(IPP_DPKG_DIR)
	./$(IPP_DPKG_DIR)/usr/local/eaton/IntelligentPowerProtector/mc2-install -install -silent -dir $(PWD)/$(IPP_INSTALL_DIR)
	chmod +x $(IPP_INSTALL_DIR)/mc2
# Remove unused desktop files.
	rm -r $(IPP_INSTALL_DIR)/desktop
# Remove webpanel certificate and private key. We generate a new unique pair in 'postinst'.
	rm $(IPP_INSTALL_DIR)/bin/key.pem $(IPP_INSTALL_DIR)/bin/cert.pem
# Build and install 'synopoweroff' wrapper.
	mkdir -p $(IPP_INSTALL_DIR)/configs/actions
	$(CC) -o $(IPP_INSTALL_DIR)/configs/actions/synopoweroff src/synopoweroff.c $(CFLAGS)
	chmod 0750 $(IPP_INSTALL_DIR)/configs/actions/synopoweroff

# 	mkdir -p $(IPP_INSTALL_DIR)/configs/actions
# 	$(CC) -o $(IPP_INSTALL_DIR)/configs/actions/server src/server.c $(CFLAGS) -pthread
# 	chmod 0750 $(IPP_INSTALL_DIR)/configs/actions/server

install: $(IPP_INSTALL_DIR)
	cp -a $(IPP_INSTALL_DIR)/* $(DESTDIR)

clean:
	rm -rf $(IPP_PKGNAME) $(IPP_DPKG_DIR) $(IPP_INSTALL_DIR)
