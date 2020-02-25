说明：
    rt5350_dev.c rt5350_dev.h rt5350_ioctl.h
         这三个文件是m3s的io驱动
         请将这三个文件拷贝到drivers/char路径下。
     并在source/vendor/RT5350/makedevlinks 文件中增加一条如下：
    mkdev   dev/rt_dev  c   10      200 $cons
     