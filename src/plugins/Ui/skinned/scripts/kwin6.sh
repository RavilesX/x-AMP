#!/bin/sh

#This script adds kwin rules for qmmp windows

if ! type kreadconfig6 > /dev/null; then
  exit 1
fi

if ! type kwriteconfig6 > /dev/null; then
  exit 1
fi

# get count of rules
count=`kreadconfig6 --file kwinrulesrc --group General --key count`
i=1
found=0

if [ -z $count ]; then
  count=0
fi

while [ $i -le $count ];
do
	# find qmmp window rule in KWin
    match=`kreadconfig6 --file kwinrulesrc --group $i --key wmclass`
    if [ "${match}" = "Qmmp" ]; then
        found=$i
        break
    fi
    i=$((i+1))
done


qmmp_create_rule(){
	id=$1

	kwriteconfig6 --file kwinrulesrc --group General --key count $id
	kwriteconfig6 --file kwinrulesrc --group $id --key Description qmmp
}


qmmp_update_rule(){
	id=$1

	kwriteconfig6 --file kwinrulesrc --group $id --key skippager true
	kwriteconfig6 --file kwinrulesrc --group $id --key skippagerrule 3
	kwriteconfig6 --file kwinrulesrc --group $id --key skipswitcher true
	kwriteconfig6 --file kwinrulesrc --group $id --key skipswitcherrule 3
	kwriteconfig6 --file kwinrulesrc --group $id --key type 5
	kwriteconfig6 --file kwinrulesrc --group $id --key typerule 2
	kwriteconfig6 --file kwinrulesrc --group $id --key types 288
	kwriteconfig6 --file kwinrulesrc --group $id --key wmclass Qmmp
	kwriteconfig6 --file kwinrulesrc --group $id --key wmclasscomplete true
	kwriteconfig6 --file kwinrulesrc --group $id --key wmclassmatch 2
}

reload_kwin_rules(){
	dbus-send --print-reply --dest=org.kde.KWin /KWin org.kde.KWin.reconfigure
}


if [ $found = "0" ]; then
	# rule not found (create new KWin window rule)
	id=$((count+1))

	qmmp_create_rule $id
	qmmp_update_rule $id
	reload_kwin_rules
fi
