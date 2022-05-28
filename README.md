# netlink_without_libnl

Testing using the kernel's netlink, more specifically nl80211 without libnl or genl.  It's shockingly easy.  Almost, as though the library just makes things much more complicated than they should be.

This project is currently a proof of concept that works.  Tested with multiple wifi devices, and Kernel 5.4.0.

So, if you are on an embedded system, and/or you have other reasons to avoid extra fluff layers, here's your demo.




