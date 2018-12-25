#include "CalcNormal.h"
void CalcNormal::SetPoints(std::vector<std::vector<float>> points) {
	this->points = points;
	this->convertPoints2PointXYZ();
}
cv::Mat CalcNormal::GetDepth() {
	convertPointXYZ2Depth();
	return depth_image;
}
cv::Mat CalcNormal::GetNormal() {
	convertPointXYZ2Normal();
	return normal_image;
}
pcl::PointCloud<pcl::PointXYZ> CalcNormal::GetPoints() {
	return this->points_cloud;
}
void CalcNormal::convertPoints2PointXYZ() {
	//pcl::PointCloud<pcl::PointXYZ> point_cloud_ptr;
	this->points_cloud.points.clear();
	for (int p = 0; p < points.size(); p++) {
		pcl::PointXYZ point;
		point.x = points.at(p).at(0) * 500;
		point.y = points.at(p).at(1) * 500;
		point.z = points.at(p).at(2) * 500;
		this->points_cloud.points.push_back(point);
	}
	this->upsample(5, 3, 1.5);
	//this->deOutlier(30, 0.0001);

}
void CalcNormal::deOutlier(int neighbour, double dev)
{
	pcl::StatisticalOutlierRemoval<pcl::PointXYZ> sor;
	sor.setInputCloud(this->points_cloud.makeShared());
	sor.setMeanK(neighbour);
	sor.setStddevMulThresh(dev);
	sor.filter(this->points_cloud);
}
void CalcNormal::upsample(float search_radius, float upsample_radius, float step) {
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud2(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr smoothedCloud(new pcl::PointCloud<pcl::PointXYZ>);
	// Smoothing object (we choose what point types we want as input and output).
	pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointXYZ> filter;
	filter.setInputCloud(points_cloud.makeShared());
	// ������������İ뾶
	filter.setSearchRadius(search_radius);
	// If true, the surface and normal are approximated using a polynomial estimation
	// (if false, only a tangent one).
	filter.setPolynomialFit(true);
	// We can tell the algorithm to also compute smoothed normals (optional).
	filter.setComputeNormals(true);
	// kd-tree object for performing searches.
	pcl::search::KdTree<pcl::PointXYZ>::Ptr kdtree;
	filter.setSearchMethod(kdtree);

	filter.setUpsamplingMethod(pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointXYZ>::SAMPLE_LOCAL_PLANE);
	filter.setUpsamplingRadius(upsample_radius);
	filter.setUpsamplingStepSize(step);

	filter.process(*smoothedCloud);
	points_cloud = *smoothedCloud;
}

void CalcNormal::convertPointXYZ2Depth() {
	double minx = this->points_cloud.points[0].x, maxx = minx, miny = this->points_cloud.points[0].y, maxy = miny, minz = this->points_cloud.points[0].z, maxz = minz;
	for(int i=0;i<this->points_cloud.points.size();i++)
	{
		minx = min(minx, (double)this->points_cloud.points.at(i).x);
		maxx = max(maxx, (double)this->points_cloud.points.at(i).x);
		miny = min(miny, (double)this->points_cloud.points.at(i).y);
		maxy = max(maxy, (double)this->points_cloud.points.at(i).y);
		minz = min(minz, (double)this->points_cloud.points.at(i).z);
		maxz = max(maxz, (double)this->points_cloud.points.at(i).z);
	}
	cv::Mat M((int)(maxx - minx + 1), (int)(maxy - miny + 1), CV_8UC1);
#pragma omp parallel for
	for (int i = 0; i<M.rows; i++)
	{
		for (int j = 0; j<M.cols; j++)
		{
			M.at<uchar>(i, j) = 0;
		}
	}
#pragma omp parallel for
	for(int i=0;i<this->points_cloud.points.size();i++)
	{
		int x = (int)(this->points_cloud.points.at(i).x - minx);
		int y = (int)(this->points_cloud.points.at(i).y - miny);
		double pixel = (this->points_cloud.points.at(i).z - minz) / (maxz - minz) * 255;
		if (pixel > M.at<uchar>(x, y))
		{
			M.at<uchar>(x, y) = (this->points_cloud.points.at(i).z - minz) / (maxz - minz) * 255;
		}

	}
	cv::medianBlur(M, M, 3);

	depth_image = M;
}
void CalcNormal::convertPointXYZ2Normal() {
	pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
	pcl::PointCloud<pcl::Normal>::Ptr pcNormal(new pcl::PointCloud<pcl::Normal>);
	pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);

	//tree->setInputCloud(this->points_cloud);
	ne.setInputCloud(this->points_cloud.makeShared());
	ne.setSearchMethod(tree);
	ne.setKSearch(20);
	ne.compute(*pcNormal);


	double minx = this->points_cloud.points[0].x, maxx = minx, miny = this->points_cloud.points[0].y, maxy = miny, minz = this->points_cloud.points[0].z, maxz = minz;
	//for (vector<pcl::PointXYZ>::iterator it = this->points_cloud->begin(); it != this->points_cloud->end(); it++)
	for(int i=0;i<this->points_cloud.points.size();i++)
	{
		minx = min(minx, (double)this->points_cloud.points.at(i).x);
		maxx = max(maxx, (double)this->points_cloud.points.at(i).x);
		miny = min(miny, (double)this->points_cloud.points.at(i).y);
		maxy = max(maxy, (double)this->points_cloud.points.at(i).y);
		minz = min(minz, (double)this->points_cloud.points.at(i).z);
		maxz = max(maxz, (double)this->points_cloud.points.at(i).z);
	}
	cv::Mat normal_image = cv::Mat::zeros((int)(maxx - minx + 1), (int)(maxy - miny + 1), CV_8UC3);
#pragma omp parallel for
	for (int i = 0; i < pcNormal->size(); ++i)
	{
		//ͳһnormal����nz>0
		if (pcNormal->points[i].normal_z < 0)
		{
			pcNormal->points[i].normal_x *= -1;
			pcNormal->points[i].normal_y *= -1;
			pcNormal->points[i].normal_z *= -1;
		}
		normal_image.at<cv::Vec3b>((int)(this->points_cloud.points[i].x - minx), (int)(this->points_cloud.points[i].y - miny))[2] = (int)((pcNormal->points[i].normal_x + 1) * 128 - 1);
		normal_image.at<cv::Vec3b>((int)(this->points_cloud.points[i].x - minx), (int)(this->points_cloud.points[i].y - miny))[1] = (int)((pcNormal->points[i].normal_y + 1) * 128 - 1);
		normal_image.at<cv::Vec3b>((int)(this->points_cloud.points[i].x - minx), (int)(this->points_cloud.points[i].y - miny))[0] = (int)((pcNormal->points[i].normal_z + 1) * 128 - 1);
	}
	cv::medianBlur(normal_image,normal_image,3);

	this->normal_image = normal_image;

}
CalcNormal::CalcNormal() {

}
//void CalcNormal::ShowPoints() {
//	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
//	viewer->setBackgroundColor(0, 0, 0);
//	viewer->addPointCloud<pcl::PointXYZ>(points_cloud, "sample cloud");
//	viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "sample cloud");
//	while (!viewer->wasStopped())
//	{
//		viewer->spinOnce(100);
//		boost::this_thread::sleep(boost::posix_time::microseconds(100000));
//	}
//}