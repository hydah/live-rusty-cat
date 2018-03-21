file=$1
tmpfile="/tmp/live-rusty-cat-result-`date +%s`"
fail_times=`grep 'first_itime 0' $1 | wc -l`
grep 'address' $1  | grep -v 'first_itime 0' > $tmpfile
suc_times=`cat $tmpfile | wc -l`

avg_handshake=9999999
avg_connection=9999999
avg_iframe=9999999
avg_e2e=9999999
duration=0
wait_time=0

if [ $suc_times -gt 0 ]; then
    # grep -Po 'connection_time.*?(,|$)' | grep -Eo  '[0-9]+'
    # address rtmp://play.jcloud.com/profiling/avatar, handshake_time 63, connection_time 334, firstItime 399, total_count 6, nonfluency_count 0, nonfluency_rate 0.00, sei_frame_count 4, e2e 202, e2relay 168, e2edge 42
    avg_handshake=$(cat $tmpfile | grep -Po 'handshake_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 9999999; else print sum/NR}')
    avg_connection=$(cat $tmpfile | grep -Po 'connection_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 9999999; else print sum/NR}')
    avg_iframe=$(cat $tmpfile | grep -Po '(first_itime|firstItime).*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 9999999; else print sum/NR}')
    avg_e2e=$(cat $tmpfile | grep -Po 'e2e.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 99999; else print sum/NR}')
    duration=$(cat $tmpfile | grep -Po 'duration.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum}')
    wait_time=$(cat $tmpfile | grep -Po 'waiting_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum}')
fi
wait_rate=$(echo "$duration $wait_time" | awk '{if ($1==0) print 100; else print $2/$1}')

echo -n `hostname`:
echo -n ' try_times '$(($fail_times+$suc_times))
echo -n '; fail_times '$fail_times
echo -n '; avg_handshake '$avg_handshake
echo -n '; avg_connection '$avg_connection
echo -n '; avg_iframe '$avg_iframe
echo -n '; avg_e2e '$avg_e2e
echo -n '; duration '$duration
echo -n '; waiting_time '$wait_time
echo    '; waiting_rate '$wait_rate
