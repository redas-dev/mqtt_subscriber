include $(TOPDIR)/rules.mk

PKG_NAME:=mqtt_sub
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/mqtt_sub
	CATEGORY:=Base system
	TITLE:=mqtt_sub
	DEPENDS:=+argp-standalone +libmosquitto +libuci +cJSON +libcurl
endef

define Package/mqtt_sub/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DIR) $(1)/etc/config

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqtt_sub $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/*.h $(1)/usr/include/
	$(INSTALL_CONF) ./files/mqtt_sub.config $(1)/etc/config/mqtt_sub
endef

$(eval $(call BuildPackage,mqtt_sub))