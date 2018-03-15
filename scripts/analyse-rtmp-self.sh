file=$1
tmpfile="/tmp/live-rusty-cat-result-`date +%s`"
fail_times=`grep 'total_count 0' $1 | wc -l`
grep 'address' $1  | grep -v 'total_count 0' > $tmpfile
suc_times=`cat $tmpfile | wc -l`

if [ $suc_times -gt 0 ]; then
    # grep -Po 'connection_time.*?(,|$)' | grep -Eo  '[0-9]+'
    # address rtmp://play.jcloud.com/profiling/avatar, handshake_time 63, connection_time 334, firstItime 399, total_count 6, nonfluency_count 0, nonfluency_rate 0.00, sei_frame_count 4, e2e 202, e2relay 168, e2edge 42
    avg_handshake=$(cat $tmpfile | grep -Po 'handshake_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 9999999; else print sum/NR}')
    avg_connection=$(cat $tmpfile | grep -Po 'connection_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 9999999; else print sum/NR}')
    avg_iframe=$(cat $tmpfile | grep -Po 'firstItime.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 9999999; else print sum/NR}')
    avg_e2e=$(cat $tmpfile | grep -Po 'e2e.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 99999; else print sum/NR}')
    total_cnt=$(cat $tmpfile | grep -Po 'total_count.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {print sum}')
    nonf_cnt=$(cat $tmpfile | grep -Po 'nonfluency_count.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {print sum}')
fi
avg_nf=$(echo "$total_cnt $nonf_cnt" | awk '{if ($1==0) print 0.0; else print $2/$1}')

echo -n `hostname`:
echo -n ' try_times '$(($fail_times+$suc_times))
echo -n '; fail_times '$fail_times
echo -n '; avg_handshake '$avg_handshake
echo -n '; avg_connection '$avg_connection
echo -n '; avg_iframe '$avg_iframe
echo -n '; avg_e2e '$avg_e2e
echo -n '; total_cnt '$total_cnt
echo -n '; nonf_cnt '$nonf_cnt
echo    '; avg_nf '$avg_nf
