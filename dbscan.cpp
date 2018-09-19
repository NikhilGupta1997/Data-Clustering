#include "nanoflann.hpp"
#include "utils.h"
#include <ctime>
#include <cstdlib>
#include <iostream>
#include "helper.cpp"
#include <time.h> 
#include <math.h>
#include <fstream>
#include <set>
#include <queue>

using namespace std;
using namespace nanoflann;

const int NOISE = -1;
const int UNCLASSIFIED = -2;

template <typename num_t>
void kdtree_demo(const size_t N)
{
	PointCloud<num_t> cloud;

	// Generate points:
	generateRandomPointCloud(cloud, N);

	// construct a kd-tree index:
	typedef KDTreeSingleIndexAdaptor<
		L2_Simple_Adaptor<num_t, PointCloud<num_t> > ,
		PointCloud<num_t>,
		3 /* dim */
		> my_kd_tree_t;

	my_kd_tree_t   index(3 /*dim*/, cloud, KDTreeSingleIndexAdaptorParams(10 /* max leaf */) );
	index.buildIndex();

#if 0
	// Test resize of dataset and rebuild of index:
	cloud.pts.resize(cloud.pts.size()*0.5);
	index.buildIndex();
#endif

	const num_t query_pt[3] = { 0.5, 0.5, 0.5};

	// ----------------------------------------------------------------
	// knnSearch():  Perform a search for the N closest points
	// ----------------------------------------------------------------
	{
		size_t num_results = 5;
		std::vector<size_t>   ret_index(num_results);
		std::vector<num_t> out_dist_sqr(num_results);

		num_results = index.knnSearch(&query_pt[0], num_results, &ret_index[0], &out_dist_sqr[0]);
		
		// In case of less points in the tree than requested:
		ret_index.resize(num_results);
		out_dist_sqr.resize(num_results);

		cout << "knnSearch(): num_results=" << num_results << "\n";
		for (size_t i = 0; i < num_results; i++)
			cout << "idx["<< i << "]=" << ret_index[i] << " dist["<< i << "]=" << out_dist_sqr[i] << endl;
		cout << "\n";
	}

	// ----------------------------------------------------------------
	// radiusSearch(): Perform a search for the points within search_radius
	// ----------------------------------------------------------------
	{
		const num_t search_radius = static_cast<num_t>(0.1);
		std::vector<std::pair<size_t,num_t> >   ret_matches;

		nanoflann::SearchParams params;
		//params.sorted = false;

		const size_t nMatches = index.radiusSearch(&query_pt[0], search_radius, ret_matches, params);

		cout << "radiusSearch(): radius=" << search_radius << " -> " << nMatches << " matches\n";
		for (size_t i = 0; i < nMatches; i++)
			cout << "idx["<< i << "]=" << ret_matches[i].first << " dist["<< i << "]=" << ret_matches[i].second << endl;
		cout << "\n";
	}

}

class DBScan{
private:
	int minPts;
	double eps;
	vector<vector<coord>> points;
	vector<vector<int>> clusters;
	vector<int> point_to_cluster;
	set<int> noise;

public:
	DBScan(int minPts, double eps, string filename){
		this->minPts = minPts;
		this->eps = eps;
		std::ifstream file(filename);

		CSVRow row;
		while(file >> row) {
	    	this->points.push_back(row.val());
		}
		int size = points.size();
		point_to_cluster.resize(size, UNCLASSIFIED);
		int clusterid = 0;
		// clusters.resize(1);
		for(int i=0;i<size;i++){
			if(point_to_cluster[i]==UNCLASSIFIED){
				if(expand_cluster(i, clusterid))
					clusterid++;
			}
		}

	}

	bool expand_cluster(int index, int clusterid){
		vector<int> seeds = regionQuery(index, eps, points);
		if(seeds.size()<minPts){
			point_to_cluster[index] = NOISE;
			noise.insert(index);
		}
		else{
			int si = seeds.size();
			vector<int> clust;
			queue<int> values;
			for(int i=0;i<si;i++){
				int ind = seeds[i];
				point_to_cluster[ind] = clusterid;
				clust.push_back(ind);
				if(ind!=index)
					values.push(ind);
			}
			while(!values.empty()){
				int ind = values.front();
				values.pop();
				vector<int> result = regionQuery(ind, eps, points);
				for(auto pt:result){
					int val = point_to_cluster[pt];
					if(val==UNCLASSIFIED || val==NOISE){
						if(val==UNCLASSIFIED && pt!=ind)
							values.push(pt);
						point_to_cluster[pt] = clusterid;
						clust.push_back(pt);
					}
				}
			}
			clusters.push_back(clust);
		}
	}



};

int main(int argc, char **argv){
	// cout << "You have entered " << argc 
 //         << " arguments:" << "\n"; 
  
 //    for (int i = 0; i < argc; ++i) 
 //        cout << argv[i] << "\n"; 

    srand(static_cast<unsigned int>(time(nullptr)));
	kdtree_demo<float>(4);
	kdtree_demo<double>(100000);
	return 0;
}