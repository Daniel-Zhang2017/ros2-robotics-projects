lio-sam online (no gnss):
ros2 launch lio_sam run.launch.py

lio-sam online (gnss):
ros2 launch lio_sam  run_gnss.launch.py

lio-sam offline (no gnss):
ros2 launch lio_sam run_offline.launch.py

lio-sam offline (gnss):
ros2 launch lio_sam  run_offline_gnss.launch.py
or
ros2 launch lio_sam  run_usegnss.launch.py

## Save map
```
ros2 service call /lio_sam/save_map lio_sam/srv/SaveMap
```
```
ros2 service call /lio_sam/save_map lio_sam/srv/SaveMap "{resolution: 0.2, destination: /Downloads/service_LOAM}"
```
