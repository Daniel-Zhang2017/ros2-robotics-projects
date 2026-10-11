from setuptools import find_packages, setup

package_name = 'nav2_application'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Daniel',
    maintainer_email='hhzhang2016@outlook.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'init_robot_pose=nav2_application.init_robot_pose:main',
            'get_robot_pose=nav2_application.get_robot_pose:main',
            'nav_to_pose=nav2_application.nav_to_pose:main',
            'waypoint_follower=nav2_application.waypoint_follower:main',
        ],
    },
)
