from setuptools import setup

package_name = 'person_detector'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', [
            'launch/camera_yolo_person.launch.py',
            'launch/camera_person_track.launch.py'
        ]),
    ],
    install_requires=['setuptools', 'ultralytics', 'torch', 'opencv-python'],
    zip_safe=True,
    maintainer='da',
    maintainer_email='da@todo.com',
    description='Subscribe /detected_image and alert when person detected',
    license='Apache‑2.0',
    entry_points={
        'console_scripts': [
            'person_detector_node = person_detector.person_detector_node:main',
        ],
    },
)
