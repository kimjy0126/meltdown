#!/bin/bash

rm -f grub_bak
rm -f grub_new
cp /etc/default/grub grub_bak

#for line in $set
while read line
do
    if [[ "$line" == *"="* ]]
    then
        cont=${line%%=*}
        if [[ "$cont" == "GRUB_CMDLINE_LINUX_DEFAULT" ]]
        then
            rcont=${line##"$cont="}
            if [[ "$rcont" == *"pti=off"* ]]
            then
                echo "pti is already off"
            else
                line="$cont""="${rcont%\"}" pti=off\""
                echo $line
            fi
        fi
    fi
    echo "$line" >> grub_new
done < grub_bak

mv grub_new /etc/default/grub
update-grub
reboot
