# /dev/heart

Let's see ... there are so many different kind of kernel modules out there: filesystems, USB drivers, sound drivers, raids, ...
But waait waaaaait ... how do I actually know how our beloved Master Tux is doing?

That's what this kernel module will do!

**Let's a do an Electrocardiography on Master Tux' heart!**:

```bash
aplay -r 44100 -f s16_le /dev/heart
```

or if you like piping stuff:

```bash
cat /dev/heart | aplay -r 44100 -f s16_le
```

## Installation

```bash
git clone https://github.com/timofurrer/devheart
cd devheart
make
sudo make insert
```

## Wow, did I really just hear Master Tux' heart?

Yes, sure!

So, we could assume that Master Tux' heart are the CPUs. Now, depending on how stressed those CPUs are, Master Tux will feel healthy or not.
See, `dmesg` for more information.

## Let's run it in production

Yeah! Great idea! Just, don't tell your boss ...
