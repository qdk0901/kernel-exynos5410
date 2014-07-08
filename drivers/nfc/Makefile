#
# Makefile for nfc devices
#

obj-$(CONFIG_PN544_NFC)		+= pn544.o
obj-$(CONFIG_PN544_HCI_NFC)	+= pn544_hci.o
obj-$(CONFIG_NFC_PN533)		+= pn533.o
obj-$(CONFIG_NFC_WILINK)	+= nfcwilink.o
obj-$(CONFIG_BCM2079X_I2C)  += bcm2079x-i2c.o
obj-$(CONFIG_SEC_NFC)	+= sec_nfc.o

ccflags-$(CONFIG_NFC_DEBUG) := -DDEBUG
