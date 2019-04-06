
OLDIFS="$IFS";
IFS=$' ' 

read_bytes=0
write_bytes=0
files_sent=0
files_received=0
max_id=0
min_id=10000000000000
clients=0
list_of_clients=""
clinets_leaved=0
while read line
do
    chunks=$line
    for x in $chunks
    do
        if [[ $next_read -eq "1" ]]
        then
            let read_bytes=read_bytes+$x
            next_read=0
        fi

        if [ $x  == "Read" ]
        then
            next_read=1
        fi
        ###############################################
        if [[ $next_write -eq "1" ]]
        then
            let write_bytes=write_bytes+$x
            next_write=0
        fi

        if [ $x  == "Wrote" ]
        then
            next_write=1
        fi
        ############################################### 
        if [[ $next_id -eq "1" ]]
        then
            if [ $x -gt $max_id ]
            then
                max_id=$x
            fi

            if [ $x -lt $min_id ]
            then
                min_id=$x
            fi
            let clients=clients+1
            list_of_clients="$list_of_clients | $x"
            next_id=0
        fi

        if [ $x  == "id" ]
        then
            next_id=1
        fi
        ############################################### 
        if [[ $x  == *"sent"* ]]
        then
            let files_sent=files_sent+1
        fi

        if [[ $x == *"received"* ]]
        then
            let files_received=files_received+1
        fi
        ###############################################
        if [ $x == "leaved" ]
        then
            let clinets_leaved=clinets_leaved+1
        fi
    done
done 

echo "Read bytes:       " $read_bytes 
echo "Write bytes:      " $write_bytes
echo "Files sent:       " $files_sent
echo "Files received:   " $files_received 
echo "Max id:           " $max_id
echo "Min id:           " $min_id
echo "Clients leaved:   " $clinets_leaved
echo "All clients:      " $clients
echo -n "List of id(s):    "
for x in $list_of_clients
do
    echo -n $x
done
echo "|"

IFS="$OLDIFS"