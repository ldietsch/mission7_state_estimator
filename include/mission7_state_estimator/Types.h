#ifndef MISSION7_STATE_ESTIMATOR_MISSION7_STATE_ESTIMATOR_TYPES_H_
#define MISSION7_STATE_ESTIMATOR_MISSION7_STATE_ESTIMATOR_TYPES_H_

#include <ros/ros.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Sparse>
#include <Eigen/LU>
#include <Eigen/SparseCholesky>
#include <Eigen/Cholesky>

//NOTE: the standard floating point precision we are using is double

// x, y, z, thx, thy, thz, b_dx, b_dy, b_dz, b_wx, b_wy, b_wz, b_ax, b_ay, b_az, biases_gyro, biases_accel
#define QUAD_STATE_SIZE 15
// x, y, theta
#define OBSTACLE_STATE_SIZE 3
// x, y, theta, timer
#define TARGET_STATE_SIZE 4


//NOTE: the covariances stored within these substates are typically not updated and
// only used for initialization
struct TargetState{
private:
	Eigen::Matrix<double, TARGET_STATE_SIZE, 1> mean;
	Eigen::Matrix<double, TARGET_STATE_SIZE, TARGET_STATE_SIZE> cov;
public:
	TargetState(){
		mean.setZero();
		cov.setZero();
	}

	TargetState(Eigen::Matrix<double, TARGET_STATE_SIZE, 1> _mean, Eigen::Matrix<double, TARGET_STATE_SIZE, TARGET_STATE_SIZE> _cov){
		this->mean = _mean;
		this->cov = _cov;
	}

	Eigen::Matrix<double, TARGET_STATE_SIZE, TARGET_STATE_SIZE> getCov(){return this->cov;}
	void setCov(Eigen::Matrix<double, TARGET_STATE_SIZE, TARGET_STATE_SIZE> _cov){this->cov = _cov;}
	Eigen::Matrix<double, TARGET_STATE_SIZE, 1> getMean(){return this->mean;}
	void setMean(Eigen::Matrix<double, TARGET_STATE_SIZE, 1> _mean){this->mean = _mean;}
};

struct ObstacleState{
private:
	Eigen::Matrix<double, OBSTACLE_STATE_SIZE, 1> mean;
	Eigen::Matrix<double, OBSTACLE_STATE_SIZE, OBSTACLE_STATE_SIZE> cov;
public:
	ObstacleState(){
		mean.setZero();
		cov.setZero();
	}

	ObstacleState(Eigen::Matrix<double, OBSTACLE_STATE_SIZE, 1> _mean, Eigen::Matrix<double, OBSTACLE_STATE_SIZE, OBSTACLE_STATE_SIZE> _cov){
		this->mean = _mean;
		this->cov = _cov;
	}

	Eigen::Matrix<double, OBSTACLE_STATE_SIZE, OBSTACLE_STATE_SIZE> getCov(){return this->cov;}
	void setCov(Eigen::Matrix<double, OBSTACLE_STATE_SIZE, OBSTACLE_STATE_SIZE> _cov){this->cov = _cov;}
	Eigen::Matrix<double, OBSTACLE_STATE_SIZE, 1> getMean(){return this->mean;}
	void setMean(Eigen::Matrix<double, OBSTACLE_STATE_SIZE, 1> _mean){this->mean = _mean;}
};

struct QuadState{
private:
	Eigen::Matrix<double, QUAD_STATE_SIZE, 1> mean;
	Eigen::Matrix<double, QUAD_STATE_SIZE, QUAD_STATE_SIZE> cov;
public:
	QuadState(){
		mean.setZero();
		cov.setZero();
	}

	QuadState(Eigen::Matrix<double, QUAD_STATE_SIZE, 1> _mean, Eigen::Matrix<double, QUAD_STATE_SIZE, QUAD_STATE_SIZE> _cov){
		this->mean = _mean;
		this->cov = _cov;
	}

	Eigen::Matrix<double, QUAD_STATE_SIZE, QUAD_STATE_SIZE> getCov(){return this->cov;}
	void setCov(Eigen::Matrix<double, QUAD_STATE_SIZE, QUAD_STATE_SIZE> _cov){this->cov = _cov;}
	Eigen::Matrix<double, QUAD_STATE_SIZE, 1> getMean(){return this->mean;}
	void setMean(Eigen::Matrix<double, QUAD_STATE_SIZE, 1> _mean){this->mean = _mean;}


};

struct GroundRobotState{
private:
	std::vector<ObstacleState> obstacle_states;
	std::vector<TargetState> target_states;

	Eigen::MatrixXd Sigma;

public:

	void removeRow(Eigen::MatrixXd& matrix, unsigned int rowToRemove)
	{
		unsigned int numRows = matrix.rows()-1;
		unsigned int numCols = matrix.cols();

		if( rowToRemove < numRows )
			matrix.block(rowToRemove,0,numRows-rowToRemove,numCols) = matrix.block(rowToRemove+1,0,numRows-rowToRemove,numCols);

		matrix.conservativeResize(numRows,numCols);
	}

	void removeColumn(Eigen::MatrixXd& matrix, unsigned int colToRemove)
	{
		unsigned int numRows = matrix.rows();
		unsigned int numCols = matrix.cols()-1;

		if( colToRemove < numCols )
			matrix.block(0,colToRemove,numRows,numCols-colToRemove) = matrix.block(0,colToRemove+1,numRows,numCols-colToRemove);

		matrix.conservativeResize(numRows,numCols);
	}

	/*
	 * ORDER: OBSTACLE, TARGETS
	 * converts this dynamic state to a vector in the order above
	 */
	Eigen::Matrix<double, Eigen::Dynamic, 1> toStateVector(){
		Eigen::Matrix<double, Eigen::Dynamic, 1> state;

		int dimensions = obstacle_states.size()*OBSTACLE_STATE_SIZE + target_states.size()*TARGET_STATE_SIZE;
		state.resize(dimensions, 1); // allocate all of the dimensions


		// set each of the obstacles
		for(int i = 0; i < obstacle_states.size(); i++){
			state.block(i * OBSTACLE_STATE_SIZE, 0, OBSTACLE_STATE_SIZE, 1) = obstacle_states.at(i).getMean();
		}

		//set each of the target states
		int start_row = obstacle_states.size() * OBSTACLE_STATE_SIZE;
		for(int i = 0; i < target_states.size(); i++){
			state.block(start_row + i * TARGET_STATE_SIZE, 0, TARGET_STATE_SIZE, 1) = target_states.at(i).getMean();
		}
	}
	/*
	 * converts a state vector into this state type
	 * ORDER: OBSTACLE, TARGETS
	 */
	void fromStateVector(Eigen::Matrix<double, Eigen::Dynamic, 1> vec){
		// make sure that you don't add extra dimensions somehow
		int dimensions = obstacle_states.size()*OBSTACLE_STATE_SIZE + target_states.size()*TARGET_STATE_SIZE;
		ROS_ASSERT(vec.rows() == dimensions && vec.cols() == 1);


		// set the obstacle states
		for(int i = 0; i < obstacle_states.size(); i++){
			this->obstacle_states.at(i).setMean(vec.block(i*OBSTACLE_STATE_SIZE, 0, OBSTACLE_STATE_SIZE, 1));
		}

		//set the target robot states
		int start_row = obstacle_states.size() * OBSTACLE_STATE_SIZE;
		for(int i = 0; i < target_states.size(); i++){
			this->target_states.at(i).setMean(vec.block(start_row + i*TARGET_STATE_SIZE, 0, TARGET_STATE_SIZE, 1));
		}

	}

	/*
	 * adds a target state at the end of the state and resizes the covariance matrix
	 * all external correlations are set to zero
	 * the covariance contained within the target state is used to set the covaraince
	 */
	void addTargetState(TargetState x){
		this->target_states.push_back(x);

		const int new_dim = obstacle_states.size()*OBSTACLE_STATE_SIZE + target_states.size()*TARGET_STATE_SIZE;
		const int old_dim = new_dim - TARGET_STATE_SIZE;

		this->Sigma.conservativeResize(new_dim, new_dim); // does not set any of the newly allocated coeffs

		// set the correlation blocks to zero
		this->Sigma.block(0, old_dim, old_dim, TARGET_STATE_SIZE).setZero();
		this->Sigma.block(old_dim, 0, TARGET_STATE_SIZE, old_dim).setZero();

		// set the targets uncertainty
		this->Sigma.block(old_dim, old_dim, TARGET_STATE_SIZE, TARGET_STATE_SIZE) = x.getCov();
	}

};

#endif
