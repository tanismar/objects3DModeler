/*
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Tanis Mar
 * email:  tanis.mar@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#ifndef __TOOLFEATEXT_H__
#define __TOOLFEATEXT_H__

// Includes
#include <iostream>
#include <math.h>
#include <vector>
#include <ctime>

// YARP - iCub libs
#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include <yarp/math/Rand.h>
#include <yarp/math/Math.h>
#include <yarp/os/Time.h>

//#include <iCub/data3D/SurfaceMeshWithBoundingBox.h>
//#include <iCub/data3D/minBoundBox.h>
//#include <iCub/data3D/RGBA.h>

#include "iCub/YarpCloud/CloudUtils.h"


//PCL libs
#include <pcl/point_cloud.h>
//#include <pcl/io/pcd_io.h>
//#include <pcl/io/ply_io.h>
#include <pcl/point_types.h>

#include <pcl/octree/octree.h>
#include <pcl/octree/octree_impl.h>
#include <pcl/common/transforms.h>
#include <pcl/console/parse.h>
#include <pcl/features/moment_of_inertia_estimation.h>
#include <pcl/features/normal_3d.h>

//for the thrift interface
#include "ToolFeat3DwithOrient.h"
#include "VoxFeat.h"
#include "Point3D.h"

#include <tool3DFeat_IDLServer.h>

/**********************************************************
    PUBLIC METHODS
/**********************************************************/

/**********************************************************/
class ToolFeatExt : public yarp::os::RFModule, public tool3DFeat_IDLServer
{
protected:
    /* module parameters */
    std::string robot;

    yarp::os::RpcServer rpcInPort;  // port to handle incoming commands
    yarp::os::RpcClient rpcVisualizerPort;      //rpc port to communicate with tool3Dshow module to display pointcloud

    yarp::os::Port      feat3DoutPort; // Port where the features of the tool are send out (as a thrift Tool3DwithOrient struct)    
    yarp::os::BufferedPort<yarp::os::Bottle> cloudsOutPort; // Port to send out the cloud as a boltte to be further processed or displayed
    yarp::os::BufferedPort<yarp::os::Bottle> cloudsInPort; // Port to send out the cloud as a boltte to be further processed or displayed
    std::string cloudpath;            // path to folder with .ply or .pcd files
    std::string cloudname;           // name of the .ply or .pcd cloud file



    /* class variables */
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_orig; // Point cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud;      // Point cloud of the transformed model
    yarp::sig::Matrix                      rotMat;     // Rotation Matrix specifying grasp pose

    bool verbose;
    int maxDepth;
    int binsPerDim;

    std::vector<yarp::os::Bottle>   models;             // Vector to contain all models considered in the experiment.

    bool closing;
    bool cloudLoaded;
    bool cloudTransformed;

    /* functions*/    
    bool loadToolModel(const std::string &tool);
    bool loadModelsFromFile(yarp::os::ResourceFinder &rf);
    bool transform2pose(const yarp::sig::Matrix& toolPose = yarp::math::eye(4,4));
    int  computeOMSEGI();
    bool sendCloud(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_in);
    std::string deg2ori(const float deg);

public:

    // RPC Accesible methods
    bool                        getFeats();
    bool                        getAllToolFeats(const int n_samples, bool pics);
    bool                        getSamples(const int n, const double deg);
    bool                        setPose(const yarp::sig::Matrix& rotMat);
    bool                        setCanonicalPose(const double deg = 0.0, const int disp = 0);

    bool                        setBinNum(const int binsN = 2);
    bool                        setDepth(const int depthN = 2);
    bool                        loadModel(const std::string& name = "cloud.ply");
    bool                        setName(const std::string& name = "cloud.ply");
    bool                        setVerbose(const std::string& verb);

    // module control //
    bool						attach(yarp::os::RpcServer &source);
    bool						quit();

    // RF modules overrides
    bool						configure(yarp::os::ResourceFinder &rf);
    bool						interruptModule();
    bool						close();
    bool						updateModule();
    double						getPeriod();
};

#endif
