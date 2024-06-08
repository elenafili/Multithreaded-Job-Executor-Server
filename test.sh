./bin/jobCommander localhost 1234 issueJob sleep 5 &
./bin/jobCommander localhost 1234 issueJob sleep 10 &
./bin/jobCommander localhost 1234 issueJob sleep 12 &
./bin/jobCommander localhost 1234 issueJob sleep 13 &
./bin/jobCommander localhost 1234 issueJob sleep 14 &
./bin/jobCommander localhost 1234 poll
sleep 6
./bin/jobCommander localhost 1234 poll

./bin/jobCommander localhost 1234 exit &
./bin/jobCommander localhost 1234 exit &
./bin/jobCommander localhost 1234 exit &
./bin/jobCommander localhost 1234 exit &
./bin/jobCommander localhost 1234 exit &
./bin/jobCommander localhost 1234 poll &
./bin/jobCommander localhost 1234 setConcurrency 10 &
./bin/jobCommander localhost 1234 issueJob sleep 100 &
./bin/jobCommander localhost 1234 stop job_2 