# /dev/heart

Let's see ... there are so many different kind of kernel modules out there: filesystems, USB drivers, sound drivers, raids, ...
But waait waaaaait ... how do I actually know how our beloved Master Tux is doing?

That's what this kernel module will do!

**Let's a do an Electrocardiography on Master Tux' heart!**:

```bash
aplay -r 44100 -f s16_le /dev/heart
```

## Installation

```bash
git clone https://github.com/timofurrer/devheart
cd devheart
make
sudo make insert
```
