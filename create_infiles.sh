#!/bin/bash



if [ "$#" -le 3 ]
then
    echo "Expected more arguments."
    exit -1
fi

if [ "$2" -lt 0 ]
then
    echo "Number of files is not valid."
    exit -2
fi

if [ "$3" -lt 0 ]
then
    echo "Number of directories is not valid."
    exit -3
fi

if [ "$4" -lt 0 ]
then
    echo "Number of levels is not valid."
    exit -4
fi

if [ ! -d  "$1" ]
then
    mkdir $1
fi

set=("A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "Y" "Z" "a" "b" "c" "d" "e" "f" "g" "h" "i" "j" "k" "l" "m" "n" "o" "p" "q" "r" "s" "t" "u" "v" "w" "x" "y" "z" "0" "1" "2" "3" "4" "5" "6" "7" "8" "9")


for ((i=0 ; i<"$3" ; i++))
do
    ran=$(($RANDOM % 62))
    string="${set[ran]}" 
    name_length=$((($RANDOM % 8) +1))
    for ((j=1 ; j<"$name_length" ;j++))
    do
        ran=$(($RANDOM % 62))
        string="$string${set[ran]}" 
    done
    rand_dir_names[i]="$string"
    #echo "$string"
done

dir_paths[0]="$1"
index=1
level=0
for i in ${rand_dir_names[@]}
do
    if [ "$level" -eq 0 ]
    then    
        full_path="$1"
        full_path="$full_path/$i"
    else
        full_path="$full_path/$i"
    fi
    echo "$full_path"
    if [ ! -d  "$full_path" ]
    then
        dir_paths[index]="$full_path"
        let index=index+1
        mkdir $full_path
    fi

    if [ "$4" -gt 0 ]
    then
        let level=level+1
    fi

    if [ "$level" -eq "$4" ]
    then
        level=0
    fi

done

counter=1
for ((i=0 ; i<"$2" ; i++))
do
    ran=$(($RANDOM % 62))
    string="${set[ran]}" 
    name_length=$((($RANDOM % 8) +1))
    for ((j=1 ; j<"$name_length" ;j++))
    do
        ran=$(($RANDOM % 62))
        string="$string${set[ran]}" 
    done
    rand_file_names[i]="${string}f$counter"
    let counter=counter+1
done



echo stop
steps=1
for i in ${rand_file_names[@]}
do
    current_steps=0
    while [ $current_steps -lt $steps ]
    do
        for j in ${dir_paths[@]}
        do
            let current_steps=current_steps+1
            if [ $current_steps -eq $steps ]
            then
                path="$j"
                break
            fi
        done
    done

    if [ ! -f  "$path/$i" ]
    then
        file="$path/$i"
        size=$((($RANDOM % 128)+1))
        size="$(( $size * 1000))"
        touch $file
        ran="$(($RANDOM % 62))"
        char="${set[ran]}"
        for((j=0 ; j<"$size" ; j++))
        do
            echo "$char"  >> $file
        done
    fi
    let steps=steps+1
    echo "$file"
done