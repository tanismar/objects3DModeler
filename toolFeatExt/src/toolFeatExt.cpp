/*
 * TOOL 3D FEATURE EXTRACTOR
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Tanis Mar
 * email: tanis.mar@iit.it
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

#include "toolFeatExt.h"

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
//using namespace iCub::ctrl;

/**********************************************************
                    PRIVATE METHODS
/**********************************************************/

/************************************************************************/
bool ToolFeatExt::loadCloud()
{
    cout << "Loading cloud from file" << path.c_str() << fname.c_str() << endl;

    // (Re-) initialize cloud transformations
    cloud_orig->clear();
    cloud->clear();
    cloudTransformed = false;
    rotMat = yarp::math::eye(4,4);

    // Load the pointcloud either from a pcd or a ply file.
    DIR *dir;
    if ((dir = opendir (path.c_str())) != NULL) {
        string::size_type idx;
        idx = fname.rfind('.');
        if(idx != std::string::npos)
        {
            string ext = fname.substr(idx+1);
            cout << "Found file with extension " << ext << endl;
            if(strcmp(ext.c_str(),"ply")==0)
            {
                printf ("Loading .ply file: %s\n", fname.c_str());
                if (pcl::io::loadPLYFile (path+fname, *cloud_orig) < 0)	{
                    PCL_ERROR("Error loading cloud %s.\n", fname.c_str());
                    return false;
                }
            }else if(strcmp(ext.c_str(),"pcd")==0)
            {
                printf ("Loading .pcd file: %s\n", fname.c_str());
                if (pcl::io::loadPCDFile (path+fname, *cloud_orig) < 0)	{
                    PCL_ERROR("Error loading cloud %s.\n", fname.c_str());
                    return false;
                }
            }else {
                PCL_ERROR("Please select a .pcd or .ply file.\n");
                return false;
            }
        }else {
            PCL_ERROR(" %s does not have valid file format.\n", fname.c_str());
            return false;
        }
    } else {
        /* could not open directory */
        printf ("can't open data directory.");
        return false;
    }
    cloudLoaded = true;
    return true;
}

/************************************************************************/
// toolPose is represented by the rotation matrix of the explored object wrt the canonical position.
// It shall be computed by a previous fucntion/module as the transformation matrix obtained when
//   registratering the single view to the canonical mode.
bool ToolFeatExt::transformFrame(const Matrix &toolPose)
{   // transformFrame() rotates the full canonical 3D model to the pose on which it has been found to be
    //    in order to obtain the pose - dependent 3D features.

    //Load cloud if ain't yet loaded
    if (!cloudLoaded){
        if (!loadCloud())
        {
            printf("Couldn't load cloud");
            return false;
        }
    }

    rotMat = toolPose;
    cloudTransformed = true;

    if (verbose){	cout << "Transform with Matrix " << endl << toolPose.toString() <<endl;	}

    // format YARP Matrix yo Eigen Matrix
    Eigen::Matrix4f TM = yarpMat2eigMat(toolPose);

    // Execute the transformation
    pcl::transformPointCloud(*cloud_orig , *cloud, TM);

    if (verbose){	printf("Transformation done \n");	}

    return true;
}

/************************************************************************/
int ToolFeatExt::computeFeats()
    {
    if (!cloudLoaded){
        if (!loadCloud())
        {
            printf("Couldn't load cloud");
            return -1;
        }
    }

    if (!cloudTransformed){
        transformFrame();
    }

    /* ===========================================================================*/
    // Get fixed bounding box to avoid octree automatically figure it out (what might change with resolution)
    pcl::MomentOfInertiaEstimation <pcl::PointXYZ> feature_extractor;
    feature_extractor.setInputCloud (cloud);
    feature_extractor.compute ();

    pcl::PointXYZ min_point_AABB;
    pcl::PointXYZ max_point_AABB;
    feature_extractor.getAABB (min_point_AABB, max_point_AABB);

    /* =========================================================================== */
    // Divide the bounding box in subregions using octree, and compute the normal histogram (EGI) in each of those subregions at different levels.

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Create instances of necessary classes

    // octree
    float minVoxSize = 0.01; // Min voxel size (max resolution) 1 cm
    pcl::octree::OctreePointCloudSearch<pcl::PointXYZ> octree(minVoxSize);

    // Normal estimation class, and pass the input dataset to it
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;

    // Create an empty kdtree representation, and pass it to the normal estimation object.
    // Its content will be filled inside the object, based on the given input dataset (as no other search surface is given).
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ> ());
    ne.setSearchMethod (tree);

    // Cloud of Normals from whole pointcloud
    pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal> ());

    // voxelCloud and voxelCloudNormals to contain the poitns on each voxel
    pcl::PointCloud<pcl::PointXYZ>::Ptr voxelCloud(new pcl::PointCloud<pcl::PointXYZ> ());
    pcl::PointCloud<pcl::Normal>::Ptr voxelCloudNormals(new pcl::PointCloud<pcl::Normal> ());

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Initialize octree
    //create octree structure
    octree.setInputCloud(cloud);
    //update bounding box automatically
    octree.defineBoundingBox(min_point_AABB.x, min_point_AABB.y, min_point_AABB.z,
                             max_point_AABB.x, max_point_AABB.y, max_point_AABB.z);
    //add points in the tree
    octree.addPointsFromInputCloud();


    // Compute all normals from the orignial cloud, and THEN subdivide into voxels (to avoid voxel boundary problems arising when computing normals voxel-wise)
    // Clear output cloud
    cloud_normals->points.clear();

    // Pass the input cloud to the normal estimation class
    ne.setInputCloud (cloud);

    // Use all neighbors in a sphere of radius of size of the voxel length
    ne.setRadiusSearch (minVoxSize/2);

    // Compute the normals
    ne.compute (*cloud_normals);


    // Initialize histogram variables
    //float pi = 3.14159;

    float rangeMin = -1;    // XXX make this automatic.
    float rangeMax = 1;     //          ´´
    float rangeValid = rangeMax - rangeMin;
    float sizeBin = rangeValid/binsPerDim;

    std::vector < std::vector < double > > featureVectorOccVox;     // Vector containing histograms for occupied voxels
    ToolFeat3DwithOrient featureVectorAllVox;     // Vector containing histograms for all voxels, also the empty ones
    featureVectorAllVox.toolname = fname;
    featureVectorAllVox.orientation = rotMat;
    //std::vector < std::vector < float > > featureVectorAllVox;     // Vector containing histograms for all voxels, also the empty ones
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Find the minimum resolution octree (depth =1) to start increase it from there

    double cBB_min_x, cBB_min_y, cBB_min_z, cBB_max_x, cBB_max_y, cBB_max_z;
    octree.getBoundingBox(cBB_min_x, cBB_min_y, cBB_min_z, cBB_max_x, cBB_max_y, cBB_max_z);
    double cBBsize = fabs(cBB_max_x - cBB_min_x);           // size of the cubic bounding box encompasing the whole pointcloud

    if(verbose){
        cout << "AA BB: (" << min_point_AABB.x << "," << min_point_AABB.y << "," << min_point_AABB.z << "), (" << max_point_AABB.x << "," <<max_point_AABB.y << "," << max_point_AABB.z << ")" << endl;
        cout << "Octree BB: (" << cBB_min_x << "," << cBB_min_y << "," <<cBB_min_z << "), (" << cBB_max_x << "," <<cBB_max_y << "," <<cBB_max_z << ")" << endl;
    }

    // re-initialize tree with new resolution
    minVoxSize = cBBsize/2;     //Update to minimum resolution
    octree.deleteTree();
    octree.setResolution(minVoxSize);
    octree.defineBoundingBox(cBB_min_x, cBB_min_y, cBB_min_z,
                             cBB_max_x, cBB_max_y, cBB_max_z);
    octree.setInputCloud(cloud);
    octree.addPointsFromInputCloud();

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Perform feature extraction iteratively on increasingly smaller voxels up to maxDepth level.
    for (int depth = 1; depth<=maxDepth; ++depth)
    {
        if(verbose){cout << endl << "====================== Depth:" << depth << " ========================== " << endl;}

        featureVectorOccVox.clear();                                // Clear the vector saving all populated histograms at depth level

        int voxPerSide = pow(2,depth);
        bool voxelOccupancy[voxPerSide][voxPerSide][voxPerSide];    // Keeps record of the position of the occupied voxels at each depth.
        for (int i = 0; i < voxPerSide; ++i)                        // initialize all positions to false
            for (int j = 0; j < voxPerSide; ++j)
                for (int k = 0; k < voxPerSide; ++k)
                   voxelOccupancy[i][j][k] = false;

        // Instantiate normal histogram at actual depth
        int numLeaves = octree.getLeafCount();      // Get the number of occupied voxels (leaves) at actual depth
        float voxelHist[numLeaves][binsPerDim][binsPerDim][binsPerDim]; // Histogram 3D matrix per voxel
        std::vector< double> featVecHist;                                 // Vector containing a seriazed version of the 3D histogram

        if(verbose){
            cout << "BB divided into " << voxPerSide << " voxels per side of size "<< minVoxSize << "," << numLeaves << " occupied." << endl;
            cout << "Looping through the  " << numLeaves << " occupied voxels." << endl;
        }

        // Extracts all the points at leaf level from the octree
        int octreePoints = 0;
        int voxelI = 0;
        int nanCount = 0;
        pcl::octree::OctreePointCloudSearch<pcl::PointXYZ>::LeafNodeIterator tree_it;
        pcl::octree::OctreePointCloudSearch<pcl::PointXYZ>::LeafNodeIterator tree_it_end = octree.leaf_end();

        // Iterate through lower level leafs (voxels).
        for (tree_it = octree.leaf_begin(depth); tree_it!=tree_it_end; ++tree_it)
        {
            //std::cout << "Finding points within voxel " << voxelI << std::endl;
            //std::cout << "Finding points within voxel " << voxelI << std::endl;

            // Clear clouds and vectors from previous voxel.
            voxelCloud->points.clear();
            voxelCloudNormals->points.clear();
            featVecHist.clear();

            // Find Voxel Bounds ...
            Eigen::Vector3f voxel_min, voxel_max;
            octree.getVoxelBounds(tree_it, voxel_min, voxel_max);

            //... and center
            pcl::PointXYZ voxelCenter;
            voxelCenter.x = (voxel_min.x() + voxel_max.x()) / 2.0f;
            voxelCenter.y = (voxel_min.y() + voxel_max.y()) / 2.0f;
            voxelCenter.z = (voxel_min.z() + voxel_max.z()) / 2.0f;

            // Mark octree leaves as occupied voxels.
            int occx, occy, occz;                                  // voxel indices
            occx = floor((voxelCenter.x - cBB_min_x) /minVoxSize); // floor because 0-indexed array
            occy = floor((voxelCenter.y - cBB_min_y) /minVoxSize);
            occz = floor((voxelCenter.z - cBB_min_z) /minVoxSize);
            //std::cout << "Voxel (" << occx << "," << occy << "," << occz << ") is occupied" << std::endl;
            voxelOccupancy[occx][occy][occz]= true;

            //std::cout << "Voxel center at ("
            //            << voxelCenter.x << " "
            //            << voxelCenter.y << " "
            //            << voxelCenter.z << ")" << std::endl;

            // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // Neighbors within voxel search
            // It assigns the search point to the corresponding leaf node voxel and returns a vector of point indices.
            // These indices relate to points which fall within the same voxel.
            std::vector<int> voxelPointsIdx;
            if (octree.voxelSearch (voxelCenter, voxelPointsIdx))
            {
                for (size_t i = 0; i < voxelPointsIdx.size (); ++i)
                {
                    // Save Points within voxel as cloud
                    voxelCloud->points.push_back(cloud->points[voxelPointsIdx[i]]);
                    voxelCloudNormals->points.push_back(cloud_normals->points[voxelPointsIdx[i]]);

                    octreePoints++;
                } // for point in voxel
            }
            //std::cout << "Voxel has " << voxelCloud->points.size() << " points. " << std::endl;


            /* =================================================================================*/
            // On each voxel compute the EGI (sphere normal histogram). Normalize and send it out as feature vector.

            // Initialize histogram for new voxel
            for (int i = 0; i < binsPerDim; ++i)
                for (int j = 0; j < binsPerDim; ++j)
                    for (int k = 0; k < binsPerDim; ++k)
                       voxelHist[voxelI][i][j][k] = 0.0f;

            // Compute normal histogram looping through normals in voxel.
            unsigned int okNormalsVox = 0;
            for (size_t i = 0; i < voxelCloudNormals->points.size (); ++i)
            {
                int xbin, ybin, zbin;
                float xn, yn, zn;

                if ((pcl::isFinite(voxelCloudNormals->points[i]))){
                    xn = voxelCloudNormals->points[i].normal_x;
                    yn = voxelCloudNormals->points[i].normal_y;
                    zn = voxelCloudNormals->points[i].normal_z;
                    //std::cout << "Normal  (" << xn << "," << yn << "," << zn << ")" << std::endl;

                    xbin = floor((xn - rangeMin) /sizeBin); // floor because 0-indexed array
                    ybin = floor((yn - rangeMin) /sizeBin);
                    zbin = floor((zn - rangeMin) /sizeBin);
                    //std::cout << "lies on bin (" << xbin << "," << ybin << "," << zbin << ")" << std::endl;

                    voxelHist[voxelI][xbin][ybin][zbin]= voxelHist[voxelI][xbin][ybin][zbin] + 1.0f;
                    okNormalsVox ++;
                }else{
                    nanCount++;
                    //std::cout << "Normal is NaN." << std::endl;

                }
            }   //- close loop over normal points in voxel
            std::cout << "." ;

            //Normalize by the total number of point in voxel so that each histogram has sum 1, independent of the number of points in the voxel.
            for (int i = 0; i < binsPerDim; ++i){
                for (int j = 0; j < binsPerDim; ++j){
                    for (int k = 0; k < binsPerDim; ++k){
                       voxelHist[voxelI][i][j][k] = voxelHist[voxelI][i][j][k] / (float) okNormalsVox;

                       // Push into feature vector per histogram
                       featVecHist.push_back(voxelHist[voxelI][i][j][k]);
                    }
                }
            }

            // Print out angular histogram
            /*
            cout << "Angular histogram: " << std::endl << "{";
            for (int i = 0; i < binsPerDim; ++i){
                cout <<  "[" ;
                for (int j = 0; j < binsPerDim; ++j){
                    for (int k = 0; k < binsPerDim; ++k)
                       cout << voxelHist[voxelI][i][j][k] << "," ;
                    cout <<  std::endl;
                }
                cout <<  "]" ;
            }
            cout <<  "}" <<std::endl;
            // */

            // Keep all histograms form occupied voxels
            featureVectorOccVox.push_back(featVecHist);

            voxelI++;

        } // close for voxel

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Print out security checks

        // Check that all methods end with the same number of points, to avoid missing or repetition.
        if(verbose){
            cout << "Point Cloud has " << cloud->points.size() << " points." << endl;
            cout << "Normals Cloud has " << cloud->points.size() << " points, of which "<< nanCount << " are NaNs" << endl;
            cout << "Octree found " << octreePoints << " points." << endl;
        }

        // Print out voxel occupancy and stream out histograms
        // /*
        cout << "Voxel Occupancy: " << std::endl << "{";
        for (int i = 0; i < voxPerSide; ++i){
            cout <<  "[" ;
            for (int j = 0; j < voxPerSide; ++j){
                for (int k = 0; k < voxPerSide; ++k){
                   cout << voxelOccupancy[i][j][k] << "," ;
                }
                cout <<  std::endl;
            }
            cout <<  "]" ;
        }
        cout <<  "}" <<std::endl;
        //*/

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Fill the complete feature vector, including histograms for empty voxels.
        int occVoxI = 0;
        for (int i = 0; i < voxPerSide; ++i){
            for (int j = 0; j < voxPerSide; ++j){
                for (int k = 0; k < voxPerSide; ++k){
                    if (voxelOccupancy[i][j][k]){        // if the voxel is occupied
                        std::vector< double > histVec;
                        histVec = featureVectorOccVox[occVoxI];
                        featureVectorAllVox.toolFeats.push_back(histVec); // copy histogram
                        occVoxI++;
                    }else{
                        std::vector< double >  zeroVector(pow(binsPerDim,3),0.0f);
                        featureVectorAllVox.toolFeats.push_back(zeroVector); // fill with the same amount of zeros
                    }
                }
            }
        }

        //if(verbose){cout <<  "Vector has a size of " << featureVectorAllVox.size() << " x " << featVecHist.size() << " at depth "<< depth <<endl;}

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Re-initialize octree with double resolution (half minVoxSize) to analyse next level
        minVoxSize = minVoxSize/2;
        octree.deleteTree();
        octree.setResolution(minVoxSize);       // re-initialize tree with new resolution
        octree.defineBoundingBox(cBB_min_x, cBB_min_y, cBB_min_z,
                                 cBB_max_x, cBB_max_y, cBB_max_z);
        octree.setInputCloud(cloud);
        octree.addPointsFromInputCloud();
    }
    if(verbose){
        // print out end feature vector of vectors
        cout << "Feature vector contains: " << endl << featureVectorAllVox.toString()<< endl;
        cout << endl << "======================Feature Extraction Done========================== " << endl;
    }

    // XXX put featureVectorAllVox in a bottle and send it out.
    feat3DoutPort.write(featureVectorAllVox);

    return 0;
}


/***************** Helper Functions *************************************/
Matrix ToolFeatExt::eigMat2yarpMat(const Eigen::MatrixXf eigMat){
    // Transforms matrices from Eigen format to YARP format
    int nrows = eigMat.rows();
    int ncols = eigMat.cols();
    Matrix yarpMat =yarp::math::zeros(nrows,ncols);
    for (int row = 0; row<nrows; ++row){
        for (int col = 0; col<ncols; ++col){
            yarpMat(row,col) = eigMat(row,col);
        }
    }
    return yarpMat;
}


Eigen::MatrixXf ToolFeatExt::yarpMat2eigMat(const Matrix yarpMat){
    // Transforms matrices from YARP format to Eigen format
    int nrows = yarpMat.rows();
    int ncols = yarpMat.cols();
    Eigen::MatrixXf eigMat;
    eigMat.resize(nrows,ncols);
    for (int row = 0; row<nrows; ++row){
        for (int col = 0; col<ncols; ++col){
            eigMat(row,col) = yarpMat(row,col);
        }
    }
    return eigMat;
}


/**********************************************************
                    PUBLIC METHODS
/**********************************************************/

// RCP Accesible via trhift.

/**********************************************************/
bool ToolFeatExt::getFeats()
{   // computes 3D oriented -normalized voxel wise EGI - tool featues.
    int ok = computeFeats();
    if (ok>=0) {
        fprintf(stdout,"3D Features computed correctly. \n");
        return true;
    } else {
        fprintf(stdout,"3D Features not computed correctly. \n");
        return false;
    }
}

bool ToolFeatExt::getSamples(const int n, const float deg)
{
    loadCloud();            // Load the cloud in its canonical position
    float maxVar = 10;      //Define the maximum variation, on degrees, wrt the given orientation angle

    yarp::math::Rand randG; // YARP random generator
    Vector degVarVec = randG.vector(n); // Generate a vector of slgiht variations to the main orientation angle
    for (int s=0;s<n;++s)
    {
        if(!setCanonicalPose(deg + degVarVec[s]*maxVar-maxVar/2))   // Transform model to close but different position
        {
            printf("Error transforming frame to canonical position . \n");
        }

        if (!computeFeats()) {
            printf("Error computing features. \n");
        }
    }
    printf("Features extracted from tool pose samples. \n");
}

/**********************************************************/
bool ToolFeatExt::setPose(const Matrix& toolPose)
{   // Rotates the tool model according tot the given rotation matrix to extract pose dependent features.
    return transformFrame(toolPose);
}

/**********************************************************/
bool ToolFeatExt::setCanonicalPose(const float deg)
{   // Rotates the tool model to canonical orientations left/front/right.
    float rad = (deg/180) *M_PI; // converse deg into rads

    Vector oy(4);   // define the rotation over the Y axis (that is the one that we consider for tool orientation -left,front,right -
    oy[0]=0.0; oy[1]=1.0; oy[2]=0.0; oy[3]= rad;

    Matrix toolPose = iCub::ctrl::axis2dcm(oy);   // from axis/angle to rotation matrix notation

    return transformFrame(toolPose);
}

/**********************************************************/
bool ToolFeatExt::bins(const int binsN)
{
    binsPerDim = binsN;
    return true;
}

/**********************************************************/
bool ToolFeatExt::depth(const int depthN)
{
    maxDepth = depthN;
    return true;
}

/**********************************************************/
bool ToolFeatExt::setName(const string& cloudname)
{   //Changes the name of the .ply file to display. Default 'cloud_merged.ply'"
    fname = cloudname;
    //cloudLoaded = false;

    //if (!cloudLoaded){
        if (!loadCloud())
        {
            printf("Couldn't load cloud.\n");
            return false;
        }
    //}
    return true;
}

/**********************************************************/
bool ToolFeatExt::setVerbose(const string& verb)
{
    if (verb == "ON"){
        verbose = true;
        fprintf(stdout,"Verbose is : %s\n", verb.c_str());
        return true;
    } else if (verb == "OFF"){
        verbose = false;
        fprintf(stdout,"Verbose is : %s\n", verb.c_str());
        return true;
    }
    fprintf(stdout,"Verbose can only be set to ON or OFF. \n");
    return false;
}

/**********************************************************/

bool ToolFeatExt::help_commands()
{
    cout << "Available commands are:" << endl;
    cout << "getFeat - computes 3D oriented -normalized voxel wise EGI - tool featues." << endl;
    cout << "getSamples (int) (float) - generate 'int' number of poses of the tool oriented to 'float' degrees (-90 to 90)." <<endl;
    cout << "setName (string) - Changes the name of the .ply file to display. Default 'cloud_merged.ply'" << endl;    
    cout << "setPose (Matrix) Rotates the tool model according tot the given rotation matrix to extract pose dependent features" << endl;
    cout << "setCanonicalPose (float) Rotates the tool model to canonical position oriented 'float'degrees (left = -90 deg)/front = 0 deg / right = 90 deg)." << endl;
    cout << "bins (int) - sets the number of bins per angular dimension (yaw-pitch-roll) used to compute the normal histogram. Total number of bins per voxel = bins^3. (Default bins = 2)" << endl;
    cout << "depth (int)- sets the number of iterative times that the bounding box will be subdivided into octants. Total number of voxels = sum(8^(1:depth)). (Default depth = 2, 72 vox)" << endl;
    cout << "setVerbose ON/OFF - Sets active the printouts of the program, for debugging or visualization."<< endl;
    cout << "help - produces this help." << endl;
    cout << "quit - closes the module." << endl;
    return true;
}

bool ToolFeatExt::quit()
{
    closing = true;
    return true;
}


/************************* RF overwrites ********************************/
/************************************************************************/
bool ToolFeatExt::configure(yarp::os::ResourceFinder &rf)
{
    // add and initialize the port to send out the features via thrift.
    string name = rf.check("name",Value("toolFeatExt")).asString().c_str();
    string robot = rf.check("robot",Value("icub")).asString().c_str();
    verbose = rf.check("verbose",Value(true)).asBool();
    if (strcmp(robot.c_str(),"icub")==0)
        path = rf.find("clouds_path").asString();
    else
        path = rf.find("clouds_path_sim").asString();
    printf("Path: %s",path.c_str());

    maxDepth = rf.check("maxDepth",Value(2)).asInt();
    binsPerDim = rf.check("depth",Value(2)).asInt();


    //open ports
    bool ret = true;
    ret =  ret && feat3DoutPort.open("/"+name+"/feats3D:o");			// Port which outputs the vector containing all the extracted features
    if (!ret){
        printf("Problems opening ports\n");
        return false;
    }

    // open rpc ports
    bool retRPC = true;
    retRPC =  retRPC && rpcInPort.open("/"+name+"/rpc:i");
    if (!retRPC){
        printf("Problems opening ports\n");
        return false;
    }
    attach(rpcInPort);

    /* Module rpc parameters */
    closing = false;

    /*Init variables*/
    cloudLoaded = false;
    cloudTransformed = false;
    fname = "cloud_merged.ply";
    cloud_orig = pcl::PointCloud<pcl::PointXYZ>::Ptr (new pcl::PointCloud<pcl::PointXYZ>);
    cloud = pcl::PointCloud<pcl::PointXYZ>::Ptr (new pcl::PointCloud<pcl::PointXYZ>);
    rotMat = yarp::math::eye(4,4);

    cout << endl << "Configuring done."<<endl;

    return true;
}


double ToolFeatExt::getPeriod()
{
    return 1; //module periodicity (seconds)
}

bool ToolFeatExt::updateModule()
{
    return !closing;
}

bool ToolFeatExt::interruptModule()
{
    closing = true;
    rpcInPort.interrupt();
    feat3DoutPort.interrupt();
    cout<<"Interrupting your module, for port cleanup"<<endl;
    return true;
}


bool ToolFeatExt::close()
{
    cout<<"Calling close function\n";
    rpcInPort.close();
    feat3DoutPort.close();
    return true;
}

/**************************** THRIFT CONTROL*********************************/
bool ToolFeatExt::attach(yarp::os::RpcServer &source)
{
    return this->yarp().attachAsServer(source);
}

    /************************************************************************/
    /************************************************************************/
int main(int argc, char * argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        printf("YARP server not available!\n");
        return -1;
    }

    ToolFeatExt module;
    ResourceFinder rf;
    rf.setDefaultContext("toolModeler");
    rf.setDefaultConfigFile("toolFeatExt.ini");
    rf.setVerbose(true);
    rf.configure(argc, argv);


    cout<<"Configure module..."<<endl;
    module.configure(rf);
    cout<<"Start module..."<<endl;
    module.runModule();

    cout<<"Main returning..."<<endl;
    return 0;
}

