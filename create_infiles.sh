#!/bin/bash

#checks for validation in arguments 
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

#set of all letters and numbers
set=("A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "Y" "Z" "a" "b" "c" "d" "e" "f" "g" "h" "i" "j" "k" "l" "m" "n" "o" "p" "q" "r" "s" "t" "u" "v" "w" "x" "y" "z" "0" "1" "2" "3" "4" "5" "6" "7" "8" "9")


for ((i=0 ; i<"$3" ; i++)) #loop to make the names of the directories
do
    ran=$(($RANDOM % 62)) #pick randomly a char from the set
    string="${set[ran]}" 
    name_length=$((($RANDOM % 8) +1)) #pick randomly how many chars will have the name of the directory
    
    for ((j=1 ; j<"$name_length" ;j++)) #loop for the directory name length
    do
        ran=$(($RANDOM % 62)) #make the name
        string="$string${set[ran]}" #put it in the end of the string
    done
    
    rand_dir_names[i]="$string" #keep the names of dirs in an array
    
done

dir_paths[0]="$1" #array to keep the paths of the directories in the order we create them
index=1
level=0
for i in ${rand_dir_names[@]} #loop to create the directories
do
    if [ "$level" -eq 0 ] #make the path
    then    
        full_path="$1" #if level is 0 we are in the dir_name 
        full_path="$full_path/$i" #dir_name/dir
    else
        full_path="$full_path/$i" #else take the previous full path and do it bigger /dir_name/dir/dir2
    fi

    if [ ! -d  "$full_path" ] #if full path doesnt exist
    then
        dir_paths[index]="$full_path" #keep the path
        let index=index+1 
        mkdir $full_path #make the directory
    fi

    if [ "$4" -gt 0 ] #increase the level
    then
        let level=level+1
    fi

    if [ "$level" -eq "$4" ] #make level 0 again
    then
        level=0
    fi

done

#loop to make the file names
for ((i=0 ; i<"$2" ; i++))
do
    ran=$(($RANDOM % 62)) 
    string="${set[ran]}"            #pick randomly a char from the set and put it in string
    name_length=$((($RANDOM % 8) +1))  #pick randomly how many chars will have the name of the file

    for ((j=1 ; j<"$name_length" ;j++)) #loop for the file name length
    do
        ran=$(($RANDOM % 62)) #pick randomly chars and put them in the end of the string
        string="$string${set[ran]}" 
    done
    
    rand_file_names[i]="${string}" #keep the names in an array
done
 
steps=1 #increase by one in every loop
for i in ${rand_file_names[@]} #loop to create the files 
do
    current_steps=0
    while [ $current_steps -lt $steps ] #loop to find the directory which the file should created with round robbin
    do                                 #increase the current steps by 1 each time for every directory path
        for j in ${dir_paths[@]} 
        do
            let current_steps=current_steps+1
            if [ $current_steps -eq $steps ] #if the current steps are equal to steps 
            then
                path="$j" #keep the directory path
                break
            fi
        done
    done

    if [ ! -f  "$path/$i" ] #if the file does not exist
    then
        file="$path/$i"
        size=$((($RANDOM % 128)+1)) #pick randomly a file size
        touch $file
        ran="$(($RANDOM % 62))" 
        chars="${set[ran]}" #pick randomly a char from the set
        for ((j=1 ; j< 999 ; j++))
        do    
            chars="$chars${set[ran]}" #make a string with 1000 times the char
        done
        for((j=0 ; j<"$size" ; j++)) #put for size times this string in file
        do
            echo "$chars"  >> $file
        done
    fi
    let steps=steps+1 #increase steps for round robbin
    
done