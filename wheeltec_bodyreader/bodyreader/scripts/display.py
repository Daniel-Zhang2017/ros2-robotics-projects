#!/usr/bin/env python3
# coding=utf-8

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import Int16
import cv2, cv_bridge
import numpy as np
from rclpy.qos import QoSProfile
from bodyreader_msg.msg import Maskdata
from bodyreader_msg.msg import Bodylist
from bodyreader_msg.msg import Bodyposture
from bodyreader_msg.msg import Lockedmaskwh
from bodyreader_msg.msg import Lockedcharrgb
from bodyreader_msg.msg import Body
import math


PI = 3.1415926535898
DEPTH_HFOV = 58.4/180*PI
DEPTH_VFOV = 45.5/180*PI
RGB_HFOV = 66.1/180*PI
RGB_VFOV = 40.2/180*PI
CHAR_THRESHOLD = 15


class Display(Node):
    def __init__(self):
        super().__init__('body_display')
        self.bridge = cv_bridge.CvBridge()
        qos = QoSProfile(depth=10)
        self.body_image_sub = self.create_subscription(Image, "/image_raw", self.image_callback,qos)
        self.mask_image_sub = self.create_subscription(Maskdata, "/body/mask", self.mask_image_callback,qos)
        self.bodylist_sub = self.create_subscription(Bodylist,"/bodylist",  self.bodylist_callback,qos)
        self.body_posture_sub = self.create_subscription(Bodyposture,"/body_posture",  self.body_posture_callback,qos)
        self.body_display_pub = self.create_publisher( Image, "/body/body_display",qos)
        self.recoveryid_pub = self.create_publisher(Int16,"/recoveryid",  qos)
        self.mask = Maskdata()
        self.bodylist = Bodylist()
        self.char_rgb = Lockedcharrgb()
        self.last_char_rgb = Lockedcharrgb()
        self.body_lockedid = 0
        self.lock_status = 0
        self.saved_char = 0
        self.saved_char_i = 0
        self.last_maskpoint = 0
        self.count_callback = 0

        #self.display_msg = Image()

    def body_posture_callback(self, msg):
        self.body_lockedid = msg.bodyid
        self.lock_status = msg.lock_status

    def bodylist_callback(self, msg):
        self.bodylist = msg

    def mask_image_callback(self, msg):
        self.mask = msg

    def image_callback(self, msg):
        image_ori = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        image = cv2.resize(image_ori, (640,480), interpolation=cv2.INTER_AREA)
        #image = cv2.flip(image_temp, 1)
        # for i in range(320*240):
        #     if self.mask.data[i] != self.last_maskpoint:
        #         #pass
        #         cv2.circle(image, ((i+1)%320, (i+1)/320), 2, (0, 0, 255))
        #     self.last_maskpoint = self.mask.data[i]

        temp = [0] *self.bodylist.count
        for i in range(self.bodylist.count):
            temp[i] = self.bodylist.bodies[i].centerofmass.x
            draw = [0] *19
            #if draw joint
            for j in range(19):
                centerx = int(self.bodylist.bodies[i].joints[j].depthposition.x )
                if centerx<40 or centerx>600:
                    continue
                centery = int(self.bodylist.bodies[i].joints[j].depthposition.y )
                if centery<40 or centery>440:
                    continue
                #cv2.circle(image, (centerx, centery), 6, (0, 255, 0), -1)
                draw[j] = 1
            #draw line 
            for a in [0,1,2,3,5,6,8,9,10,11,13,14]:
                if draw[a] == 1 and draw[a+1] == 1:
                    pt1_x = int(self.bodylist.bodies[i].joints[a].depthposition.x )
                    pt1_y = int(self.bodylist.bodies[i].joints[a].depthposition.y )
                    pt2_x = int(self.bodylist.bodies[i].joints[a+1].depthposition.x )
                    pt2_y = int(self.bodylist.bodies[i].joints[a+1].depthposition.y )
                    image = cv2.line(image, (pt1_x, pt1_y), (pt2_x, pt2_y), (255,255,255), 3)
            for b in [5,8]:
                if draw[1] == 1 and draw[b] == 1:
                    pt1_x = int(self.bodylist.bodies[i].joints[1].depthposition.x )
                    pt1_y = int(self.bodylist.bodies[i].joints[1].depthposition.y )
                    pt2_x = int(self.bodylist.bodies[i].joints[b].depthposition.x )
                    pt2_y = int(self.bodylist.bodies[i].joints[b].depthposition.y )
                    image = cv2.line(image, (pt1_x, pt1_y), (pt2_x, pt2_y), (255,255,255), 3)    
            if draw[9] == 1 and draw[13] == 1:
                pt1_x = int(self.bodylist.bodies[i].joints[9].depthposition.x )
                pt1_y = int(self.bodylist.bodies[i].joints[9].depthposition.y )
                pt2_x = int(self.bodylist.bodies[i].joints[13].depthposition.x )
                pt2_y = int(self.bodylist.bodies[i].joints[13].depthposition.y )
                image = cv2.line(image, (pt1_x, pt1_y), (pt2_x, pt2_y), (255,255,255), 3)
            #draw joint
            for c in range(19):
                if draw[c] == 1 :
                    centerx = int(self.bodylist.bodies[i].joints[c].depthposition.x )
                    centery = int(self.bodylist.bodies[i].joints[c].depthposition.y )
                    image = cv2.circle(image, (centerx, centery), 6, (0, 255, 0), -1)

            #make a mask ---record locked_body characteristics
            if self.lock_status == 2 and self.bodylist.bodies[i].bodyid == self.body_lockedid  :

                if (self.bodylist.bodies[i].joints[8].worldposition.z > 1600
                    and self.bodylist.bodies[i].joints[8].depthposition.x > 80
                    and self.bodylist.bodies[i].joints[8].depthposition.x < 560 ):

                    locked_mask, width_l, width_r, height_t, height_b = self.mask_for_char(self.bodylist.bodies[i], msg)

                    if locked_mask.shape[0]!=0 and locked_mask.shape[1]!=0 and locked_mask.shape[2]!=0:
                        mask_b = locked_mask[:,:,0]
                        mask_g = locked_mask[:,:,1]
                        mask_r = locked_mask[:,:,2]
                        self.char_rgb.r = int(np.mean(mask_r))
                        self.char_rgb.g = int(np.mean(mask_g))
                        self.char_rgb.b = int(np.mean(mask_b))
                        self.saved_char = 1
                        self.saved_char_i = i
                        print(self.char_rgb)
                        print("-------------")
                        image = cv2.rectangle(image, (width_l,height_t), (width_r,height_b), (255,0,0), 4)


            elif self.lock_status == 1 and self.saved_char == 1 :
                if self.bodylist.bodies[i].joints[8].worldposition.z != 0:
                    check_mask, width_l, width_r, height_t, height_b = self.mask_for_char(self.bodylist.bodies[i], msg)
                    if check_mask.shape[0]!=0 and check_mask.shape[1]!=0 and check_mask.shape[2]!=0:
                        mean = Lockedcharrgb()
                        mean.r = int(np.mean(check_mask[:,:,2]))
                        mean.g = int(np.mean(check_mask[:,:,1]))
                        mean.b = int(np.mean(check_mask[:,:,0]))
                        #print(mean)
                        #print("+++++++++++++++++++++++")
                        # self.char_rgb =  self.char_vec[0]
                        if (self.char_rgb.r-CHAR_THRESHOLD < mean.r < self.char_rgb.r+CHAR_THRESHOLD
                            and self.char_rgb.g-CHAR_THRESHOLD < mean.g < self.char_rgb.g+CHAR_THRESHOLD
                            and self.char_rgb.b-CHAR_THRESHOLD < mean.b < self.char_rgb.b+CHAR_THRESHOLD):
                            #pass
                            recoveryid_msg = Int16()
                            recoveryid_msg.data = self.bodylist.bodies[i].bodyid
                            self.recoveryid_pub.publish(recoveryid_msg)


        if self.lock_status == 2 :
            if self.saved_char_i < len(temp):
                lock_x = temp[self.saved_char_i]
                for d in range(self.bodylist.count):
                    if d != self.saved_char_i:
                        if 100 < abs(lock_x - temp[d]):
                            self.char_rgb = self.last_char_rgb
                            break
        self.last_char_rgb = self.char_rgb


        #image_msg = cv2.resize(image, (320,240), interpolation=cv2.INTER_AREA)
        self.body_display_pub.publish(self.bridge.cv2_to_imgmsg(image, "bgr8"))
        #cv2.imshow("Body_Display", image)
        #cv2.waitKey(1)

        

    def mask_for_char(self, body, image_msg):
        bodycenterx = int(body.joints[8].depthposition.x  )
        bodycentery = int(body.joints[8].depthposition.y  )
        locked_mask_width  = int(250/body.joints[8].worldposition.z * image_msg.width/math.tan(DEPTH_HFOV) )
        locked_mask_height  = int(350/body.joints[8].worldposition.z * image_msg.height/math.tan(DEPTH_VFOV) )
        width_l = int(bodycenterx-locked_mask_width/2)
        width_r = int(bodycenterx+locked_mask_width/2)
        height_t = int(bodycentery-locked_mask_height*2/3 )
        height_b = int(bodycentery+locked_mask_height/3)
        image_temp1 = self.bridge.imgmsg_to_cv2(image_msg, desired_encoding='bgr8')
        image_temp2 = cv2.resize(image_temp1, (640,480), interpolation=cv2.INTER_AREA)
        #image_temp2 = cv2.flip(image_temp1, 1)
        locked_mask = image_temp2[ height_t:height_b , width_l:width_r ]
        return locked_mask, width_l, width_r, height_t, height_b


if __name__ == '__main__':
    rclpy.init()
    display = Display()
    try:
        rclpy.spin(display)
    finally:
        display.destroy_node()
        rclpy.shutdown()
        print('exception')

