# Root FS

I generated the rootfs using a simple yocto configuration (for branch scharthgap).

I include the very simple essential configuration in the yocto subdir, but
I don't describe the compiling instructions here (for now).

Compiling the rootfs took about an hour on my 8-Core PC.

## Post adjustments

After the root image extracted, the /etc must be adjusted.
I was too lazy to include these changes into the yocto.

The most important is the /etc/inittab. I include this file here.

With this at least you can log-in, enter root, without password (standard yocto).

There were no proper init scripts in the yocto-generated rootfs, so I created these manually:

```
/etc/rcS.d/S01syslog    -> ../init.d/syslog
/etc/rcS.d/S40network   -> ../init.d/networking
/etc/rcS.d/S50dropbear  -> ../init.d/dropbear
/etc/rcS.d/S99rc_local  -> ../rc.local
```

You sould create the K versions for these as well in /etc/rc6.d
