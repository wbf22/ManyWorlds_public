# make sure to build the game first with 'debug game' or something in the launch.json
```
sudo sh -c "echo 1 > /proc/sys/kernel/perf_event_paranoid"
perf record -F 50 --call-graph dwarf ./Godot_v4.5-stable_linux.x86_64
perf record -F 50 --call-graph dwarf bin/no_godot_test
```

## run this after recording the game (2nd one for smaller threshold)
```
perf report --stdio > bottlenecks
perf report --percent-limit 1 --stdio > bottlenecks
```

## to attach to a process for a certain amount of time
```
timeout 60s perf record -p $pid -g -o perf.data
```

# for mac with xcode installed
```
xcrun xctrace record --template "Time Profiler" --launch bin/no_godot_test
```

