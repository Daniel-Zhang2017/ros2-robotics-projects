import os
import glob
from setuptools import setup

package_name = 'sys_status_monitor'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        # Comment out launch file line temporarily if you don't need it now
        ('share/' + package_name + '/launch', ['launch/sys_status.launch.py']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='da',
    maintainer_email='da@todo.todo',
    description='System status monitor package',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'sys_status_sub = sys_status_monitor.subscriber_node:main',
            'battery_alert = sys_status_monitor.alert_node:main',
            'battery_bridge = sys_status_monitor.battery_bridge_node:main',
            
        ],
    },
)
