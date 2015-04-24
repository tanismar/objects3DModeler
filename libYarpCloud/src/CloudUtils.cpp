
#include <iCub/YarpCloud/CloudUtils.h>

using namespace std;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::YarpCloud; 


bool CloudUtils::loadCloud(const string& cloudpath, const string& cloudname, pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_to)
{
    cloud_to->clear();

    cout << "Attempting to load " << (cloudpath + cloudname).c_str() << "... "<< endl;
    // Load the pointcloud either from a pcd or a ply file.
    DIR *dir;

    // Check that directory exists
    if ((dir = opendir (cloudpath.c_str())) == NULL) {
        printf ("can't open data directory.");
        return false;
    }

    // Check that the file has a type
    string::size_type idx;
    idx = cloudname.rfind('.');
    if(idx == std::string::npos) {
        PCL_ERROR(" %s does not have valid file format.\n", cloudname.c_str());
        return false;
    }

    // Check that the file type is valid
    string ext = cloudname.substr(idx+1);
    cout << "Found file with extension " << ext << endl;

    if(strcmp(ext.c_str(),"ply")==0)        // Check if it is .ply
    {
        printf ("Loading .ply file: %s\n", cloudname.c_str());
        if (pcl::io::loadPLYFile (cloudpath + cloudname, *cloud_to) < 0)	{
            PCL_ERROR("Error loading cloud %s.\n", cloudname.c_str());
            return false;
        }
    }else if(strcmp(ext.c_str(),"pcd")==0) // Check if it is .pcd
    {
        printf ("Loading .pcd file: %s\n", cloudname.c_str());
        if (pcl::io::loadPCDFile (cloudpath + cloudname, *cloud_to) < 0)	{
            PCL_ERROR("Error loading cloud %s.\n", cloudname.c_str());
            return false;
        }
    }else {
        PCL_ERROR("Please select a .pcd or .ply file.\n");
        return false;
    }

    return true;
}


/************************************************************************/
void CloudUtils::savePointsPly(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, const string& savepath, const string& savename, const int addNum)
{
    stringstream s;
    s.str("");
    if (addNum >= 0){
        s << savepath + "/" + savename.c_str() << addNum;
    } else {
        s << savepath + "/" + savename.c_str();
    }

    string filename = s.str();
    string filenameNumb = filename+".ply";
    ofstream plyfile;
    plyfile.open(filenameNumb.c_str());
    plyfile << "ply\n";
    plyfile << "format ascii 1.0\n";
    plyfile << "element vertex " << cloud->width <<"\n";
    plyfile << "property float x\n";
    plyfile << "property float y\n";
    plyfile << "property float z\n";
    plyfile << "property uchar diffuse_red\n";
    plyfile << "property uchar diffuse_green\n";
    plyfile << "property uchar diffuse_blue\n";
    plyfile << "end_header\n";

    for (unsigned int i=0; i<cloud->width; i++)
        plyfile << cloud->at(i).x << " " << cloud->at(i).y << " " << cloud->at(i).z << " " << (int)cloud->at(i).r << " " << (int)cloud->at(i).g << " " << (int)cloud->at(i).b << "\n";

    plyfile.close();

    cout << "Cloud saved in file: " << filenameNumb.c_str() << endl;
}

void CloudUtils::mesh2cloud(const iCub::data3D::SurfaceMeshWithBoundingBox& cloudB, pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud)
{   // Converts mesh from a bottle into pcl pointcloud.
    for (size_t i = 0; i<cloudB.mesh.points.size(); ++i)
    {
        pcl::PointXYZRGB pointrgb;
        pointrgb.x=cloudB.mesh.points.at(i).x;
        pointrgb.y=cloudB.mesh.points.at(i).y;
        pointrgb.z=cloudB.mesh.points.at(i).z;
        if (i<cloudB.mesh.rgbColour.size())
        {
            int32_t rgb= cloudB.mesh.rgbColour.at(i).rgba;
            pointrgb.rgba=rgb;
            pointrgb.r = (rgb >> 16) & 0x0000ff;
            pointrgb.g = (rgb >> 8)  & 0x0000ff;
            pointrgb.b = (rgb)       & 0x0000ff;
        }
        else
            pointrgb.rgb=0;

        cloud->push_back(pointrgb);
    }
    printf("Mesh fromatted as Point Cloud \n");
}


void CloudUtils::cloud2mesh(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, iCub::data3D::SurfaceMeshWithBoundingBox& meshB, const string &cloudname)
{   // Converts pointcloud to surfaceMesh bottle.

    meshB.mesh.points.clear();
    meshB.mesh.rgbColour.clear();
    meshB.mesh.meshName = cloudname;
    for (unsigned int i=0; i<cloud->width; i++)
    {
        meshB.mesh.points.push_back(iCub::data3D::PointXYZ(cloud->at(i).x,cloud->at(i).y, cloud->at(i).z));
        meshB.mesh.rgbColour.push_back(iCub::data3D::RGBA(cloud->at(i).rgba));
    }
    iCub::data3D::BoundingBox BB = iCub::data3D::MinimumBoundingBox::getMinimumBoundingBox(cloud);
    meshB.boundingBox = BB.getBoundingBox();
    return;
}


Matrix CloudUtils::eigMat2yarpMat(const Eigen::MatrixXf eigMat)
{   // Transforms matrices from Eigen format to YARP format
    int nrows = eigMat.rows();
    int ncols = eigMat.cols();
    Matrix yarpMat = zeros(nrows,ncols);
    for (int row = 0; row<nrows; ++row){
        for (int col = 0; col<ncols; ++col){
            yarpMat(row,col) = eigMat(row,col);
        }
    }
    return yarpMat;
}



Eigen::MatrixXf CloudUtils::yarpMat2eigMat(const Matrix yarpMat){
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
