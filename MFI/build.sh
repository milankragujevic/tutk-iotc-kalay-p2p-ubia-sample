mipsel-linux-gcc -D_ANDROID_USB_TEST -D_ANDROID_USB_TEST2 -o android_usb android_usb.c -L. -lusb
mipsel-linux-strip android_usb

cp android_usb ../../document/python/android_usb