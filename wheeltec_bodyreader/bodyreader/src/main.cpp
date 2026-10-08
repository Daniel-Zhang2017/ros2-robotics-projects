// This file is part of the Orbbec Astra SDK [https://orbbec3d.com]
// Copyright (c) 2015-2017 Orbbec 3D
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
// Be excellent to each other.
#include <astra/capi/astra.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <key_handler.h>
#include <rclcpp/rclcpp.hpp>
#include "bodyreader_msg/msg/bodylist.hpp"
#include "bodyreader_msg/msg/maskdata.hpp"
#include "sensor_msgs/msg/image.hpp"
#include <vector>

rclcpp::Publisher<bodyreader_msg::msg::Bodylist>::SharedPtr bodylist_Pub;

rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_Pub;
rclcpp::Publisher<bodyreader_msg::msg::Maskdata>::SharedPtr mask_Pub;
//ros::Publisher test_Pub;

void print_color(astra_colorframe_t colorFrame)
{
    sensor_msgs::msg::Image image_msgs;

    astra_image_metadata_t metadata;
    astra_rgb_pixel_t* colorData_rgb;

    uint32_t colorByteLength;

    astra_colorframe_get_data_rgb_ptr(colorFrame, &colorData_rgb, &colorByteLength);

    astra_colorframe_get_metadata(colorFrame, &metadata);

    int width = metadata.width;
    int height = metadata.height;
    size_t index = ((width * (height / 2)) + (width / 2));

    astra_frame_index_t frameIndex;
    astra_colorframe_get_frameindex(colorFrame, &frameIndex);

    astra_rgb_pixel_t middle = colorData_rgb[index];
    printf("color frameIndex: %d  r: %d    g: %d    b: %d \n", frameIndex, (int)(middle.r), (int)(middle.g), (int)(middle.b));

    //image_msgs.header.seq = frameIndex;
    image_msgs.height = metadata.height/2;
    //printf("height = %d\n",image_msgs.height);
    image_msgs.width = metadata.width/2;
    image_msgs.encoding = "rgb8";
    image_msgs.is_bigendian = 0;
    image_msgs.step = image_msgs.width * 3;
    //printf("step = %d\n",image_msgs.step);
    image_msgs.header.frame_id = "cam";
    /*image_msgs.data.resize(image_msgs.step*image_msgs.height);
    printf("size = %d\n",image_msgs.data.size());*/
    for (int i = 0; i < metadata.height * metadata.width; i++)
    {
        if( i%2 == 1  || (int(i/640))%2 == 1) continue;

        image_msgs.data.push_back(colorData_rgb[i].r);
        image_msgs.data.push_back(colorData_rgb[i].g);
        image_msgs.data.push_back(colorData_rgb[i].b);
    }
    //std::cout << image_msgs.data.size() << std::endl;
    //printf("size = %d\n",image_msgs.data.size());
    image_Pub->publish(image_msgs);
    
}



void output_body_mask(astra_bodyframe_t bodyFrame)
{
    //sensor_msgs::Image test_msgs;
    bodyreader_msg::msg::Maskdata mask_msgs;
    astra_bodymask_t bodyMask;

    const astra_status_t rc = astra_bodyframe_bodymask(bodyFrame, &bodyMask);
    if (rc != ASTRA_STATUS_SUCCESS)
    {
        printf("Error %d in astra_bodyframe_bodymask()\n", rc);
        return;
    }


    const int32_t centerIndex = bodyMask.width / 2 + bodyMask.width * bodyMask.height / 2;
    printf("Body mask: width: %d height: %d center value: %d\n",
        bodyMask.width,
        bodyMask.height,
        bodyMask.data[centerIndex]);
    int j = 0;
    for (int i = 0; i < bodyMask.height * bodyMask.width; i++)
    {
        if( i%2 == 1  || (int(i/640))%2 == 1) continue;
        //if( bodyMask.data[i] == 0) continue;
        mask_msgs.data[j] = bodyMask.data[i];
        j++;
    }
    //printf("++++++++ == %d\n", mask_msgs.data[160*120]);
    /*test_msgs.height = 240;
    test_msgs.width = 320;
    test_msgs.encoding = "rgb8";
    test_msgs.step = 320 * 3;
    test_msgs.is_bigendian = 0;
    test_msgs.header.frame_id = "test";
    for (int a = 0; a < 320 * 240; a++)
    {
        test_msgs.data.push_back(mask_msgs.data[a]);
        test_msgs.data.push_back(0);
        test_msgs.data.push_back(0);
    }
    test_Pub.publish(test_msgs);*/

    mask_Pub->publish(mask_msgs);


}

void output_bodyframe_info(astra_bodyframe_t bodyFrame)
{
    astra_bodyframe_info_t info;

    const astra_status_t rc = astra_bodyframe_info(bodyFrame, &info);
    if (rc != ASTRA_STATUS_SUCCESS)
    {
        printf("Error %d in astra_bodyframe_info()\n", rc);
        return;
    }

    // width and height of floor mask, body mask, and the size of depth image
    // that joint depth position is relative to.
    const int32_t width = info.width;
    const int32_t height = info.height;

    printf("BodyFrame info: Width: %d Height: %d\n",
        width,
        height);
}


void output_joint(const int32_t bodyId, const astra_joint_t* joint)
{
    // jointType is one of ASTRA_JOINT_* which exists for each joint type
    const astra_joint_type_t jointType = joint->type;

    // jointStatus is one of:
    // ASTRA_JOINT_STATUS_NOT_TRACKED = 0,
    // ASTRA_JOINT_STATUS_LOW_CONFIDENCE = 1,
    // ASTRA_JOINT_STATUS_TRACKED = 2,
    const astra_joint_status_t jointStatus = joint->status;

    const astra_vector3f_t* worldPos = &joint->worldPosition;

    // depthPosition is in pixels from 0 to width and 0 to height
    // where width and height are member of astra_bodyframe_info_t
    // which is obtained from astra_bodyframe_info().
    const astra_vector2f_t* depthPos = &joint->depthPosition;

    printf("Body %u Joint %d status %d @ world (%.1f, %.1f, %.1f) depth (%.1f, %.1f)\n",
           bodyId,
           jointType,
           jointStatus,
           worldPos->x,
           worldPos->y,
           worldPos->z,
           depthPos->x,
           depthPos->y);
}


void output_bodies(astra_bodyframe_t bodyFrame)
{
    int i;
    astra_body_list_t bodyList;
    bodyreader_msg::msg::Bodylist bodylist_msg;
    const astra_status_t rc = astra_bodyframe_body_list(bodyFrame, &bodyList);
    if (rc != ASTRA_STATUS_SUCCESS)
    {
        printf("Error %d in astra_bodyframe_body_list()\n", rc);
        return;
    }
    bodylist_msg.count = bodyList.count;
    for(i = 0; i < bodyList.count; ++i)
    {   

        astra_body_t* body = &bodyList.bodies[i];

        // Pixels in the body mask with the same value as bodyId are
        // from the same body.
        bodylist_msg.bodies[i].bodyid = body->id;
        //printf("+++++++++++++++bodyId = %d\n", bodyId);
        const astra_vector3f_t* centerofmass = &body->centerOfMass;
        bodylist_msg.bodies[i].centerofmass.x = centerofmass->x;
        bodylist_msg.bodies[i].centerofmass.y = centerofmass->y;
        bodylist_msg.bodies[i].centerofmass.z = centerofmass->z;
        printf("+++++++++++++++++++++centerofmass->z = %.1f\n", centerofmass->z);
        for(int j = 0; j < 19; ++j)
        { 
          const astra_joint_t* joint = &body->joints[j];
          bodylist_msg.bodies[i].joints[j].type = joint->type ;
          const astra_vector2f_t* depthPosition = &joint->depthPosition;
          bodylist_msg.bodies[i].joints[j].depthposition.x = depthPosition->x ;
          bodylist_msg.bodies[i].joints[j].depthposition.y = depthPosition->y ;
          const astra_vector3f_t* worldPosition = &joint->worldPosition;
          bodylist_msg.bodies[i].joints[j].worldposition.x = worldPosition->x ;
          bodylist_msg.bodies[i].joints[j].worldposition.y = worldPosition->y ;
          bodylist_msg.bodies[i].joints[j].worldposition.z = worldPosition->z ;
        }

    }
    bodylist_Pub->publish(bodylist_msg);
}

void output_bodyframe(astra_bodyframe_t bodyFrame)
{

    output_body_mask(bodyFrame);

    //output_bodyframe_info(bodyFrame);

    output_bodies(bodyFrame);
}

int main(int argc, char* argv[])
{
    bool body_stream;
    bool rgb_stream;

    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("main");

    node->declare_parameter<bool>("rgb_stream", false);
    node->declare_parameter<bool>("body_stream", true);
    node->get_parameter("rgb_stream", rgb_stream);
    node->get_parameter("body_stream", body_stream);

    set_key_handler();

    astra_initialize();

    const char* licenseString = "<INSERT LICENSE KEY HERE>";
    

    orbbec_body_tracking_set_license(licenseString);

    astra_streamsetconnection_t sensor;

    astra_streamset_open("device/default", &sensor);

    astra_reader_t reader;
    astra_reader_create(sensor, &reader);

    astra_bodystream_t bodyStream;
    astra_reader_get_bodystream(reader, &bodyStream);

    astra_depthstream_t colorStream;
    astra_reader_get_colorstream(reader, &colorStream);

    if (body_stream)
    {
        bodylist_Pub = node->create_publisher<bodyreader_msg::msg::Bodylist>("/bodylist", 1);   
        mask_Pub = node->create_publisher<bodyreader_msg::msg::Maskdata>("/body/mask", 1);
        astra_stream_start(bodyStream);
    }
    
    if (rgb_stream)
    {
        image_Pub = node->create_publisher<sensor_msgs::msg::Image>("/image_raw", 1);
        astra_stream_start(colorStream);
    }
    

    do
    {
        astra_update();

        astra_reader_frame_t frame;
        astra_status_t rc = astra_reader_open_frame(reader, 0, &frame);

        if (rc == ASTRA_STATUS_SUCCESS)
        {
            if (body_stream)
            {
                astra_bodyframe_t bodyFrame;
                astra_frame_get_bodyframe(frame, &bodyFrame);

                astra_frame_index_t frameIndex;
                astra_bodyframe_get_frameindex(bodyFrame, &frameIndex);
                printf("Frame index: %d\n", frameIndex);

                output_bodyframe(bodyFrame);
            }

            printf("----------------------------\n");
            if (rgb_stream)
            {
                astra_colorframe_t colorFrame;
                astra_frame_get_colorframe(frame, &colorFrame);
                print_color(colorFrame);
            }

            astra_reader_close_frame(&frame);
        }

    } while (shouldContinue);

    astra_reader_destroy(&reader);
    astra_streamset_close(&sensor);

    astra_terminate();
}
