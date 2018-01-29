#include <moveit_cartesian_plan_plugin/widgets/path_planning_widget.h>
#include <moveit_cartesian_plan_plugin/point_tree_model.h>
#include <moveit_cartesian_plan_plugin/generate_cartesian_path.h>

namespace moveit_cartesian_plan_plugin
{
	namespace widgets {

		PathPlanningWidget::PathPlanningWidget(std::string ns):
				param_ns_(ns)
		{
			/**
			 * Constructor which calls the init() function.
			 */

			init();
		}

		PathPlanningWidget::~PathPlanningWidget()
		{

		}

		void PathPlanningWidget::init()
		{

			/**
			 * Initializing the RQT UI. Setting up the default values for the UI components:
             * - Default Values for the MoveIt and Cartesian Path
             * - Validators for the MoveIt and Cartesian Path entries
             *   - the MoveIt planning time allowed range is set from 1.0 to 1000.0 seconds
             *   - the MoveIt StepSize allowed range is set from 0.001 to 0.1 meters
             *   - the Jump Threshold for the IK solutions is set from 0.0 to 10000.0
             *   .
             *   .
      		 */

			ui_.setupUi(this);

			selected_waypoint_ = -1 ;

			//set validators for the entries
			ui_.lnEdit_PlanTime->setValidator(new QDoubleValidator(1.0,100.0,2,ui_.lnEdit_PlanTime));
			ui_.lnEdit_StepSize->setValidator(new QDoubleValidator(0.001,0.1,3,ui_.lnEdit_StepSize));
			ui_.lnEdit_JmpThresh->setValidator(new QDoubleValidator(0.0,1000.0,3,ui_.lnEdit_JmpThresh));

			//set progress bar when loading way-points from a yaml file. Could be nice when loading large way-points files
			ui_.progressBar->setRange(0,100);
			ui_.progressBar->setValue(0);
			ui_.progressBar->hide();

			QStringList headers;
			headers<<tr("Point")<<tr("Position (m)")<<tr("Orientation (deg)");
			PointTreeModel *model = new PointTreeModel(headers,"");
			ui_.treeView->setModel(model);
			ui_.btn_LoadPath->setToolTip(tr("Load Way-Points from a file"));
			ui_.btn_SavePath->setToolTip(tr("Save Way-Points to a file"));
			ui_.btnAddPoint->setToolTip(tr("Add a new Way-Point"));

			ui_.combo_DOF_FT->addItem("X");
			ui_.combo_DOF_FT->addItem("Y");
			ui_.combo_DOF_FT->addItem("Z");

			ui_.combo_DOF_FT->addItem("Rx");
			ui_.combo_DOF_FT->addItem("Ry");
			ui_.combo_DOF_FT->addItem("Rz");

			//connect(ui_.treeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)),this,SLOT(selectedPoint(const QItemSelection & selected, const QItemSelection & deselected)));
			connect(ui_.treeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex& )), this, SLOT(selectedPoint(const QModelIndex& , const QModelIndex&)));
			connect(ui_.treeView->model(), SIGNAL(itemValueChanged(const QModelIndex&, const QVariant&)), this, SLOT(treeViewDataChanged(const QModelIndex&, const QVariant&)));
			connect(ui_.targetPoint, SIGNAL(clicked()), this, SLOT(sendCartTrajectoryParamsFromUI()));
			connect(ui_.targetPoint, SIGNAL(clicked()), this, SLOT(parseWayPointBtn_slot()));
			connect(ui_.btn_LoadPath, SIGNAL(clicked()), this, SLOT(loadPointsFromFile()));
			connect(ui_.btn_SavePath, SIGNAL(clicked()), this, SLOT(savePointsToFile()));
			connect(ui_.btn_ClearAllPoints, SIGNAL(clicked()), this, SLOT(clearAllPoints_slot()));

			connect(ui_.btn_moveToHome, SIGNAL(clicked()),this, SLOT(moveToHomeFromUI()));
			connect(ui_.combo_planGroup, SIGNAL(currentIndexChanged(int)), this,SLOT(selectedPlanGroup(int)));
			connect(ui_.btn_SendCartParams, SIGNAL(clicked()), this, SLOT(setCartesianImpedanceParamsUI()));
			connect(ui_.btn_setFT, SIGNAL(clicked()), this, SLOT(setCartesianFTParamsUI()));

			connect(ui_.newWaypointX,  SIGNAL(valueChanged(double)), this, SLOT(newWaypointValueChanged(double)));
			connect(ui_.newWaypointY,  SIGNAL(valueChanged(double)), this, SLOT(newWaypointValueChanged(double)));
			connect(ui_.newWaypointZ,  SIGNAL(valueChanged(double)), this, SLOT(newWaypointValueChanged(double)));
			connect(ui_.newWaypointRx, SIGNAL(valueChanged(double)), this, SLOT(newWaypointValueChanged(double)));
			connect(ui_.newWaypointRy, SIGNAL(valueChanged(double)), this, SLOT(newWaypointValueChanged(double)));
			connect(ui_.newWaypointRz, SIGNAL(valueChanged(double)), this, SLOT(newWaypointValueChanged(double)));

			// see if the user want to have cartesian impedance
			connect(ui_.chk_CartImpedance , SIGNAL(stateChanged(int)), this, SLOT(withCartImpedanceStateChanged(int)));
			// see if the user want to have cartesian impedance
			connect(ui_.chk_EnableFT , SIGNAL(stateChanged(int)), this, SLOT(withFTControl(int)));

			//see check the status of each checkbox for enabling F/T or Cartesian Impedance
			if(ui_.chk_CartImpedance->isChecked()) {
				ui_.group_Impedance->setEnabled(true);
			}
			else {
				ui_.group_Impedance->setEnabled(true);
			}

			if(ui_.chk_EnableFT->isChecked()) {
				ui_.combo_DOF_FT->setEnabled(true);
				ui_.txt_FTValue->setEnabled(true);
				ui_.txt_FTStiffness->setEnabled(true);
				ui_.btn_setFT->setEnabled(true);
			}
			else {
				ui_.combo_DOF_FT->setEnabled(false);
				ui_.txt_FTValue->setEnabled(false);
				ui_.txt_FTStiffness->setEnabled(false);
				ui_.btn_setFT->setEnabled(false);
			}

		}

		void PathPlanningWidget::withCartImpedanceStateChanged(int state)
		{
			if(state)
			{

				ROS_INFO("User has enabled impedance");
				ui_.group_Impedance->setEnabled(true);
				//	pWidget->setEnabled(true);
			}
			else
			{
				ROS_INFO("User has disabled impedance");
				ui_.group_Impedance->setEnabled(false);
			}
		}

		void PathPlanningWidget::withFTControl(int state)
		{
			if(state)
			{
				ROS_INFO("User has enabled Force/Torque Control");
				ui_.combo_DOF_FT->setEnabled(true);
				ui_.txt_FTValue->setEnabled(true);
				ui_.txt_FTStiffness->setEnabled(true);
				ui_.btn_setFT->setEnabled(true);
			}
			else
			{
				ROS_INFO("User has disabled Force/Torque Control");
				ui_.combo_DOF_FT->setEnabled(false);
				ui_.txt_FTValue->setEnabled(false);
				ui_.txt_FTStiffness->setEnabled(false);
				ui_.btn_setFT->setEnabled(false);
			}

		}

		void PathPlanningWidget::getCartPlanGroup(std::vector< std::string > group_names)
		{
			ROS_INFO("setting the name of the planning group in combo box");
			int lenght_group = group_names.size();

			for(int i=0;i<lenght_group;i++)
			{
				ui_.combo_planGroup->addItem(QString::fromStdString(group_names[i]));
			}
		}

		void PathPlanningWidget::selectedPlanGroup(int index)
		{
			Q_EMIT sendSendSelectedPlanGroup(index);
		}

		void PathPlanningWidget::sendCartTrajectoryParamsFromUI()
		{
			/**
			 * This function takes care of sending the User Entered parameters from the RQT to the Cartesian Path Planner.
			 */

			double plan_time_,cart_step_size_,cart_jump_thresh_;
			bool moveit_replan_,avoid_collisions_;

			plan_time_        = ui_.lnEdit_PlanTime->text().toDouble();
			cart_step_size_   = ui_.lnEdit_StepSize->text().toDouble();
			cart_jump_thresh_ = ui_.lnEdit_JmpThresh->text().toDouble();

			moveit_replan_    = ui_.chk_AllowReplanning->isChecked();
			avoid_collisions_ = ui_.chk_AvoidColl->isChecked();

			Q_EMIT cartesianPathParamsFromUI_signal(plan_time_,cart_step_size_,cart_jump_thresh_,moveit_replan_,avoid_collisions_);

		}

		void PathPlanningWidget::initTreeView()
		{
			/**
			 * Initialize the Qt TreeView and set the initial value of the User Interaction arrow.
			 */

			QAbstractItemModel *model = ui_.treeView->model();

			//model->setData(model->index(0,0,QModelIndex()),QVariant("add_point_button"),Qt::EditRole);
		}

		std::string PathPlanningWidget::getWaypointName(const int row_index) const {
			QAbstractItemModel *model = ui_.treeView->model();
			QVariant name = model->data(model->index(row_index, 0), Qt::EditRole);
			return name.toString().toStdString();
		}

		void PathPlanningWidget::selectedPoint(const QModelIndex& current, const QModelIndex& previous)
		{
			/**
			 * Get the selected point from the TreeView.
			 * This is used for updating the information of the lineEdit which informs gives the number of the currently selected Way-Point.
			 **/

			QModelIndex parent = current ;
			while (parent.parent() != QModelIndex()) {
				parent = parent.parent();
			}

			selected_waypoint_ = parent.row();

			ROS_INFO_STREAM("Selected Waypoint: "<<selected_waypoint_<<" ("<<getWaypointName(selected_waypoint_)<<")");
		}

		void PathPlanningWidget::on_btnAddPoint_clicked()
		{
			/**
			 * Function for adding new Way-Point from the RQT Widget.
			 * The user can set the position and orientation of the Way-Point by entering their values in the LineEdit fields.
			 * This function is connected to the AddPoint button click() signal and sends the addPoint(point_pos) to inform the RViz enviroment that a new Way-Point has been added.
			 **/

			ROS_INFO_STREAM("Add point button clicked");

			// create transform
			Waypoint point_pos("New Point "+std::to_string(name_counter_++), getNewWaypointInputValue());

			Q_EMIT addPoint(point_pos);
		}

		void PathPlanningWidget::on_addCurrentPositionPointButton_clicked() {
			double x,y,z,rx,ry,rz;
			x = ui_.currentPositionX->value();
			y = ui_.currentPositionY->value();
			z = ui_.currentPositionZ->value();
			rx = DEG2RAD(ui_.currentPositionRx->value());
			ry = DEG2RAD(ui_.currentPositionRy->value());
			rz = DEG2RAD(ui_.currentPositionRz->value());

			// // create transform
			tf::Transform point_pos( tf::Transform(tf::createQuaternionFromRPY(rx,ry,rz),tf::Vector3(x,y,z)));
			Q_EMIT addPoint(Waypoint("New Point "+std::to_string(name_counter_++), point_pos));
		}

		void PathPlanningWidget::on_deleteWaypointButton_clicked()
		{
			/**
			 * Function for deleting a Way-Point from the RQT GUI.
			 * The name of the Way-Point that needs to be deleted corresponds to the selected_point_ member.
			 * This slot is connected to the Remove Point button signal. After completion of this function a signal is
			 * send to Inform the RViz enviroment that a Way-Point has been deleted from the RQT Widget.
			 **/

			ROS_INFO_STREAM("Deleting row "<<selected_waypoint_<<" of "<<ui_.treeView->model()->rowCount());

			if((0 <= selected_waypoint_) && (selected_waypoint_ < ui_.treeView->model()->rowCount())) {
				int copy = selected_waypoint_ ;
				selected_waypoint_ = -1 ;

				Q_EMIT pointDeleteUI_signal(copy);
			}
		}

		void PathPlanningWidget::insertRow(const Waypoint& point_pos, const int count)
		{
			/**
			 * Whenever we have a new Way-Point inserted either from the RViz or the RQT Widget the the TreeView needs
			 * to update the information and insert new row that corresponds to the new insered point.
			 * This function takes care of parsing the data receieved from the RViz or the RQT widget and creating new
			 * row with the appropriate data format and Children. One for the position giving us the current position
			 * of the Way-Point in all the axis.
			 * One child for the orientation giving us the Euler Angles of each axis.
			 **/

			ROS_INFO_STREAM("inserting new row in the TreeView at pos "<<count);
			QAbstractItemModel *model = ui_.treeView->model();

			//convert the quartenion to roll pitch yaw angle
			tf::Vector3 p = point_pos.pose_.getOrigin();
			tfScalar rx,ry,rz;
			point_pos.pose_.getBasis().getRPY(rx,ry,rz,1);

			//int len = ui_.treeView->model()->rowCount() ;

			if (count <= 0) {
				model->insertRow(count, QModelIndex());
				//count = -1;
			}
			else {
				model->insertRow(count, model->index(count, 0));
			}

			//else
			//{

				/*if(!model->insertRow(count,model->index(count, 0)))  //&& count==0
				{
					return;
				}*/

				//set the strings of each axis of the position
				QString pos_x = QString::number(p.x());
				QString pos_y = QString::number(p.y());
				QString pos_z = QString::number(p.z());

				//repeat that with the orientation
				QString orient_x = QString::number(RAD2DEG(rx));
				QString orient_y = QString::number(RAD2DEG(ry));
				QString orient_z = QString::number(RAD2DEG(rz));


			// add a child to the last inserted item. First add children in the treeview that
			// are just telling the user that if he expands them he can see details about the position and
			// orientation of each point
			QModelIndex ind = model->index(count, 0);
			model->insertRows(0, 2, ind);
			QModelIndex chldind_pos = model->index(0, 0, ind);
			QModelIndex chldind_orient = model->index(1, 0, ind);

			model->insertRows(0, 3, chldind_pos);
			model->insertRows(0, 3, chldind_orient);

			{
				const QSignalBlocker blocker(model) ;

				model->setData(model->index(count,0), QVariant(point_pos.name_.c_str()), Qt::EditRole);
				model->setData(chldind_pos, QVariant("Position"), Qt::EditRole);
				model->setData(chldind_orient, QVariant("Orientation"), Qt::EditRole);

				//*****************************Set the children for the position****************************************
				// now add information about each child separately. For the position we have coordinates for X,Y,Z axis.
				// therefore we add 3 rows of information

				// next we set up the data for each of these columns. First the names
				model->setData(model->index(0, 0, chldind_pos), QVariant("X:"), Qt::EditRole);
				model->setData(model->index(1, 0, chldind_pos), QVariant("Y:"), Qt::EditRole);
				model->setData(model->index(2, 0, chldind_pos), QVariant("Z:"), Qt::EditRole);

				// second we add the current position information, for each position axis separately
				model->setData(model->index(0, 1, chldind_pos), QVariant(pos_x), Qt::EditRole);
				model->setData(model->index(1, 1, chldind_pos), QVariant(pos_y), Qt::EditRole);
				model->setData(model->index(2, 1, chldind_pos), QVariant(pos_z), Qt::EditRole);
				//******************************************************************************************************

				//*****************************Set the children for the orientation*************************************
				// now we repeat everything again,similar as the position for adding the children for the orientation
				// next we set up the data for each of these columns. First the names
				model->setData(model->index(0, 0, chldind_orient), QVariant("Rx:"), Qt::EditRole);
				model->setData(model->index(1, 0, chldind_orient), QVariant("Ry:"), Qt::EditRole);
				model->setData(model->index(2, 0, chldind_orient), QVariant("Rz:"), Qt::EditRole);

				// second we add the current position information, for each position axis separately
				model->setData(model->index(0, 2, chldind_orient), QVariant(orient_x), Qt::EditRole);
				model->setData(model->index(1, 2, chldind_orient), QVariant(orient_y), Qt::EditRole);
				model->setData(model->index(2, 2, chldind_orient), QVariant(orient_z), Qt::EditRole);
				//******************************************************************************************************
			}

			ui_.treeView->update(ind);
		}

		void PathPlanningWidget::removeRow(int index)
		{
			/**
			 * When the user deletes certain Way-Point either from the RViz or the RQT Widget the TreeView needs to
			 *  delete that particular row and update the state of the TreeWidget.
			 */

			QAbstractItemModel *model = ui_.treeView->model();

			ROS_INFO_STREAM("Deleting point at index: "<< index<<" (name: "<<getWaypointName(index)<<")") ;

			model->removeRow(index,QModelIndex());

			for(int i=index+1; i<model->rowCount(); i++) {
				model->setData(model->index((i-1),0,QModelIndex()),QVariant((i-1)),Qt::EditRole);
			}

			//check how to properly set the selection
			ui_.treeView->selectionModel()->setCurrentIndex(model->index((model->rowCount()-1),0,QModelIndex()),QItemSelectionModel::ClearAndSelect);
		}

		tf::Transform PathPlanningWidget::getNewWaypointInputValue() {
			double x, y, z ;
			x = ui_.newWaypointX->value();
			y = ui_.newWaypointY->value();
			z = ui_.newWaypointZ->value();
			tf::Vector3 origin(x, y, z);

			double rx,ry,rz;
			rx = DEG2RAD(ui_.newWaypointRx->value());
			ry = DEG2RAD(ui_.newWaypointRy->value());
			rz = DEG2RAD(ui_.newWaypointRz->value());
			tf::Quaternion q = tf::createQuaternionFromRPY(rx,ry,rz);

			return tf::Transform(q, origin) ;
		}

		Waypoint PathPlanningWidget::getWaypointFromTreeView(const int index) {

			ROS_INFO_STREAM("Reading tree view item in row "<<index);

			//qRegisterMetaType<std::string>("std::string");
			QAbstractItemModel* model = ui_.treeView->model();

			QModelIndex parent = model->index(index, 0, QModelIndex());

			QModelIndex chldind_pos    = model->index(0, 0, parent.sibling(parent.row(), 0));
			QModelIndex chldind_orient = model->index(1, 0, parent.sibling(parent.row(), 0));

			// read out data
			QVariant pos_x = model->data(model->index(0, 1, chldind_pos), Qt::EditRole);
			QVariant pos_y = model->data(model->index(1, 1, chldind_pos), Qt::EditRole);
			QVariant pos_z = model->data(model->index(2, 1, chldind_pos), Qt::EditRole);

			QVariant orient_x = model->data(model->index(0, 2, chldind_orient), Qt::EditRole);
			QVariant orient_y = model->data(model->index(1, 2, chldind_orient), Qt::EditRole);
			QVariant orient_z = model->data(model->index(2, 2, chldind_orient), Qt::EditRole);

			tf::Vector3 pos(pos_x.toDouble(), pos_y.toDouble(), pos_z.toDouble());

			tfScalar rx,ry,rz;
			rx = DEG2RAD(orient_x.toDouble());
			ry = DEG2RAD(orient_y.toDouble());
			rz = DEG2RAD(orient_z.toDouble());

			tf::Transform pose =  tf::Transform(tf::createQuaternionFromRPY(rx,ry,rz), pos);

			return Waypoint(getWaypointName(parent.row()), pose) ;
		}

		void PathPlanningWidget::pointPosUpdated_slot(const Waypoint& waypoint, const int index)
		{
			/**
			 * When the user updates the position of the Way-Point or the User Interactive Marker, the information in
			 * the TreeView also needs to be updated to correspond to the current pose of the InteractiveMarkers.
			 */

			QAbstractItemModel *model = ui_.treeView->model();

			tf::Vector3 p = waypoint.pose_.getOrigin();
			tfScalar rx,ry,rz;
			waypoint.pose_.getBasis().getRPY(rx,ry,rz,1);

			rx = RAD2DEG(rx);
			ry = RAD2DEG(ry);
			rz = RAD2DEG(rz);

			// set the strings of each axis of the position
			QString pos_x = QString::number(p.x());
			QString pos_y = QString::number(p.y());
			QString pos_z = QString::number(p.z());

			// repeat that with the orientation
			QString orient_x = QString::number(rx);
			QString orient_y = QString::number(ry);
			QString orient_z = QString::number(rz);

			// get indices of tree items
			QModelIndex parent_index = model->index(index, 0);
			QModelIndex child_index_pos = model->index(0, 0, parent_index);
			QModelIndex child_index_orient = model->index(1, 0, parent_index);

			{
				//********************* update the positions and orientations of the children as well ******************

				// prevent Tree Model from sending new events while being updated
				const QSignalBlocker blocker(model);

				// set waypoint name
				model->setData(parent_index, QVariant(waypoint.name_.c_str()), Qt::EditRole);

				// second we add the current position information, for each position axis separately
				model->setData(model->index(0, 1, child_index_pos), QVariant(pos_x), Qt::EditRole);
				model->setData(model->index(1, 1, child_index_pos), QVariant(pos_y), Qt::EditRole);
				model->setData(model->index(2, 1, child_index_pos), QVariant(pos_z), Qt::EditRole);

				// second we add the current position information, for each position axis separately
				model->setData(model->index(0, 2, child_index_orient), QVariant(orient_x), Qt::EditRole);
				model->setData(model->index(1, 2, child_index_orient), QVariant(orient_y), Qt::EditRole);
				model->setData(model->index(2, 2, child_index_orient), QVariant(orient_z), Qt::EditRole);

				//******************************************************************************************************
			}

			//ui_.treeView->update();
			ui_.treeView->update(parent_index);

			ui_.treeView->update(model->index(0, 1, child_index_pos));
			ui_.treeView->update(model->index(1, 1, child_index_pos));
			ui_.treeView->update(model->index(2, 1, child_index_pos));

			ui_.treeView->update(model->index(0, 1, child_index_orient));
			ui_.treeView->update(model->index(1, 1, child_index_orient));
			ui_.treeView->update(model->index(2, 1, child_index_orient));
		}

		void PathPlanningWidget::treeViewDataChanged(const QModelIndex& item, const QVariant& value)
		{
			/*! This function handles the user interactions in the TreeView Widget.
			 *	The function captures an event of data change and updates the information in the TreeView and the RViz
			 *  environment.
			*/

			ROS_INFO_STREAM("Data changed. item: {row: "<<item.row()<<"; column: "<<item.column()<<"}");

			if (item == QModelIndex()) {
				return ;
			}

			qRegisterMetaType<std::string>("std::string");
			QAbstractItemModel* model = ui_.treeView->model();

			QModelIndex parent = item ;
			while (parent.parent() != QModelIndex()) {
				parent = parent.parent();
			}

			Waypoint waypoint = getWaypointFromTreeView(parent.row());

			Q_EMIT pointPosUpdated_signal(waypoint, parent.row());
		}

		void PathPlanningWidget::parseWayPointBtn_slot()
		{
			/**
			 * Letting know the Cartesian Path Planner Class that the user has pressed the Execute Cartesian Path
			 * button.
			 **/

			Q_EMIT parseWayPointBtn_signal();
		}

		void PathPlanningWidget::loadPointsFromFile()
		{
			/**
			 * Slot that takes care of opening a previously saved Way-Points yaml file.
			 * Opens Qt Dialog for selecting the file, opens the file and parses the data.
			 * After reading and parsing the data from the file, the information regarding the pose of the Way-Points
			 * is send to the RQT and the RViz so they can update their enviroments.
			 **/

			QString fileName = QFileDialog::getOpenFileName(this,
															tr("Open Way Points File"), "",
															tr("Way Points (*.yaml);;All Files (*)"));

			if (fileName.isEmpty()) {
				ui_.tabWidget->setEnabled(true);
				ui_.progressBar->hide();
				return;
			}
			else {
				ui_.tabWidget->setEnabled(false);
				ui_.progressBar->show();
				QFile file(fileName);

				if (!file.open(QIODevice::ReadOnly)) {
					QMessageBox::information(this, tr("Unable to open file"),
											 file.errorString());
					file.close();
					ui_.tabWidget->setEnabled(true);
					ui_.progressBar->hide();
					return;
				}
				// clear all the scene before loading all the new points from the file!!
				clearAllPoints_slot();

				ROS_INFO_STREAM("Opening the file: "<<fileName.toStdString());
				std::string fin(fileName.toStdString());

				YAML::Node doc;
				doc = YAML::LoadFile(fin);
				// define double for percent of completion
				double percent_complete;
				int end_of_doc = doc.size();

				for (size_t i = 0; i < end_of_doc; i++) {
					std::string name;
					geometry_msgs::Pose pose;
					tf::Transform pose_tf;

					double x,y,z,rx, ry, rz;
					name = doc[i]["name"].as<std::string>();
					x  = doc[i]["point"][0].as<double>();
					y  = doc[i]["point"][1].as<double>();
					z  = doc[i]["point"][2].as<double>();
					rx = doc[i]["point"][3].as<double>();
					ry = doc[i]["point"][4].as<double>();
					rz = doc[i]["point"][5].as<double>();

					rx = DEG2RAD(rx);
					ry = DEG2RAD(ry);
					rz = DEG2RAD(rz);

					pose_tf = tf::Transform(tf::createQuaternionFromRPY(rx,ry,rz),tf::Vector3(x,y,z));

					percent_complete = (i+1)*100/end_of_doc;
					ui_.progressBar->setValue(percent_complete);
					Q_EMIT addPoint(Waypoint(name, pose_tf));
				}
				ui_.tabWidget->setEnabled(true);
				ui_.progressBar->hide();
			}
		}

		void PathPlanningWidget::savePointsToFile()
		{
			/**
			 * Just inform the RViz environment that Save Way-Points button has been pressed.
			 **/

			Q_EMIT saveToFileBtn_press();
		}

		void PathPlanningWidget::clearAllPoints_slot()
		{
			/**
			 * Clear all the Way-Points from the RViz environment and the TreeView.
			 **/
			QAbstractItemModel *model = ui_.treeView->model();
			model->removeRows(0,model->rowCount());
			selected_waypoint_ = -1 ;
			/*tf::Transform t;
			t.setIdentity();
			insertRow(t,0);*/

			Q_EMIT clearAllPoints_signal();
		}

		void PathPlanningWidget::setAddPointUIStartPos(const std::string robot_model_frame,const tf::Transform end_effector)
		{
			/**
			 * Setting the default values for the Add New Way-Point from the RQT.
			 * The information is taken to correspond to the pose of the loaded Robot end-effector.
			 **/

			tf::Vector3 p = end_effector.getOrigin();
			tfScalar rx,ry,rz;
			end_effector.getBasis().getRPY(rx,ry,rz,1);

			rx = RAD2DEG(rx);
			ry = RAD2DEG(ry);
			rz = RAD2DEG(rz);

			// set up the starting values for the lineEdit of the positions
			ui_.newWaypointX->setValue(p.x());
			ui_.newWaypointY->setValue(p.y());
			ui_.newWaypointZ->setValue(p.z());
			// set up the starting values for the lineEdit of the orientations, in Euler angles
			ui_.newWaypointRx->setValue(rx);
			ui_.newWaypointRy->setValue(ry);
			ui_.newWaypointRz->setValue(rz);
		}

		void PathPlanningWidget::updateCurrentPositionDisplay(const std::string, const tf::Transform end_effector)
		{
			/*! Setting the default values for the Add New Way-Point from the RQT.
				The information is taken to correspond to the pose of the loaded Robot end-effector.
			*/

			tf::Vector3 p = end_effector.getOrigin();
			tfScalar rx,ry,rz;
			end_effector.getBasis().getRPY(rx,ry,rz,1);

			rx = RAD2DEG(rx);
			ry = RAD2DEG(ry);
			rz = RAD2DEG(rz);

			// set up the starting values for the lineEdit of the positions
			ui_.currentPositionX->setValue(p.x());
			ui_.currentPositionY->setValue(p.y());
			ui_.currentPositionZ->setValue(p.z());
			// set up the starting values for the lineEdit of the orientations, in Euler angles
			ui_.currentPositionRx->setValue(rx);
			ui_.currentPositionRy->setValue(ry);
			ui_.currentPositionRz->setValue(rz);
		}

		void PathPlanningWidget::cartesianPathStartedHandler()
		{
			/**
			 * Disable the RQT Widget when the Cartesian Path is executing.
			 **/
			ui_.tabWidget->setEnabled(false);
			ui_.targetPoint->setEnabled(false);
		}

		void PathPlanningWidget::cartesianPathFinishedHandler()
		{
			/**
			 * Enable the RQT Widget when the Cartesian Path execution is completed.
			 **/
			ui_.tabWidget->setEnabled(true);
			ui_.targetPoint->setEnabled(true);

		}

		void PathPlanningWidget::cartPathCompleted_slot(double fraction)
		{
			/**
			 * Get the information of what is the percentage of completion of the Planned Cartesian path from the
			 * Cartesian Path Planner class and display it in Qt label.
			 **/
			fraction = fraction*100.0;
			fraction = std::floor(fraction * 100 + 0.5)/100;

			ui_.lbl_cartPathCompleted->setText("Cartesian path " + QString::number(fraction) + "% completed.");
		}

		void PathPlanningWidget::moveToHomeFromUI()
		{
			sendCartTrajectoryParamsFromUI();
			Q_EMIT moveToHomeFromUI_signal();
		}

		void PathPlanningWidget::setCartesianImpedanceParamsUI()
		{
			cartesian_impedance_msgs::SetCartesianImpedancePtr cart_vals(new cartesian_impedance_msgs::SetCartesianImpedance);

			//stiffness Ttranslational
			cart_vals->stiffness.translational.x = ui_.txt_Stiffness_X->text().toDouble();
			cart_vals->stiffness.translational.y = ui_.txt_Stiffness_Y->text().toDouble();
			cart_vals->stiffness.translational.z = ui_.txt_Stiffness_Z->text().toDouble();
			//stiffness Rotational
			cart_vals->stiffness.rotational.x = ui_.txt_Stiffness_RX->text().toDouble();
			cart_vals->stiffness.rotational.y = ui_.txt_Stiffness_RY->text().toDouble();
			cart_vals->stiffness.rotational.z = ui_.txt_Stiffness_RZ->text().toDouble();

			//damping Ttranslational
			cart_vals->damping.translational.x = ui_.txt_Damping_X->text().toDouble();
			cart_vals->damping.translational.y = ui_.txt_Damping_Y->text().toDouble();
			cart_vals->damping.translational.z = ui_.txt_Damping_Z->text().toDouble();
			//damping Rotational
			cart_vals->damping.rotational.x = ui_.txt_Damping_RX->text().toDouble();
			cart_vals->damping.rotational.y = ui_.txt_Damping_RY->text().toDouble();
			cart_vals->damping.rotational.z = ui_.txt_Damping_RZ->text().toDouble();

			//Maximum Cartesian Velocity Linear
			cart_vals->max_cart_vel.set.linear.x = ui_.txt_MaxVel_X->text().toDouble();
			cart_vals->max_cart_vel.set.linear.y = ui_.txt_MaxVel_Y->text().toDouble();
			cart_vals->max_cart_vel.set.linear.z = ui_.txt_MaxVel_Z->text().toDouble();
			//Maximum Cartesian Velocity Angular
			cart_vals->max_cart_vel.set.angular.x = ui_.txt_MaxVel_RX->text().toDouble();
			cart_vals->max_cart_vel.set.angular.y = ui_.txt_MaxVel_RY->text().toDouble();
			cart_vals->max_cart_vel.set.angular.z = ui_.txt_MaxVel_RZ->text().toDouble();


			//Maximum Control Force Linear
			cart_vals->max_ctrl_force.set.force.x = ui_.txt_MaxCtrlForce_X->text().toDouble();
			cart_vals->max_ctrl_force.set.force.y = ui_.txt_MaxCtrlForce_Y->text().toDouble();
			cart_vals->max_ctrl_force.set.force.z = ui_.txt_MaxCtrlForce_Z->text().toDouble();
			//Maximum Control Force Angular
			cart_vals->max_ctrl_force.set.torque.x = ui_.txt_MaxCtrlForce_RX->text().toDouble();
			cart_vals->max_ctrl_force.set.torque.y = ui_.txt_MaxCtrlForce_RY->text().toDouble();
			cart_vals->max_ctrl_force.set.torque.z = ui_.txt_MaxCtrlForce_RZ->text().toDouble();

			//Maximum Cartesian Path Deviation Translation
			cart_vals->max_path_deviation.translation.x = ui_.txt_MaxPathDev_X->text().toDouble();
			cart_vals->max_path_deviation.translation.y = ui_.txt_MaxPathDev_Y->text().toDouble();
			cart_vals->max_path_deviation.translation.z = ui_.txt_MaxPathDev_Z->text().toDouble();
			//Maximum Cartesian Path Deviation Rotation
			cart_vals->max_path_deviation.rotation.x = ui_.txt_MaxPathDev_RX->text().toDouble();
			cart_vals->max_path_deviation.rotation.y = ui_.txt_MaxPathDev_RY->text().toDouble();
			cart_vals->max_path_deviation.rotation.z = ui_.txt_MaxPathDev_RZ->text().toDouble();

			//NullSpace reduntant joint parameters
			cart_vals->null_space_params.stiffness.push_back(ui_.txt_NullSpace_Stiffness->text().toDouble());
			cart_vals->null_space_params.damping.push_back(ui_.txt_NullSpace_Damping->text().toDouble());

			Q_EMIT setCartesianImpedanceParamsUI_signal(cart_vals);

			cart_vals->null_space_params.damping.clear();
			cart_vals->null_space_params.stiffness.clear();
		}

		void PathPlanningWidget::setCartesianFTParamsUI()
		{
			cartesian_impedance_msgs::SetCartesianForceCtrlPtr ft_vals(new cartesian_impedance_msgs::SetCartesianForceCtrl);
			QByteArray dof = ui_.combo_DOF_FT->currentText().toLatin1();
			ft_vals->DOF = dof.data();

			ft_vals->force 		 = ui_.txt_FTValue->text().toDouble();
			ft_vals->stiffness = ui_.txt_FTStiffness->text().toDouble();

			setCartesianFTParamsUI_signal(ft_vals);
		}

		void PathPlanningWidget::on_copyCurrentPoseButton_clicked() {
			ui_.newWaypointX->setValue(ui_.currentPositionX->value());
			ui_.newWaypointY->setValue(ui_.currentPositionY->value());
			ui_.newWaypointZ->setValue(ui_.currentPositionZ->value());

			// set up the starting values for the lineEdit of the orientations, in Euler angles
			ui_.newWaypointRx->setValue(ui_.currentPositionRx->value());
			ui_.newWaypointRy->setValue(ui_.currentPositionRy->value());
			ui_.newWaypointRz->setValue(ui_.currentPositionRz->value());
		}

		void PathPlanningWidget::on_moveToNewPositionButton_clicked() {
			sendCartTrajectoryParamsFromUI();

			double rx,ry,rz;
			rx = DEG2RAD(ui_.newWaypointRx->value());
			ry = DEG2RAD(ui_.newWaypointRy->value());
			rz = DEG2RAD(ui_.newWaypointRz->value());

			tf::Quaternion q = tf::createQuaternionFromRPY(rx,ry,rz);

			geometry_msgs::Pose pose ;

			pose.position.x = ui_.newWaypointX->value();
			pose.position.y = ui_.newWaypointY->value();
			pose.position.z = ui_.newWaypointZ->value();

			pose.orientation.w = q.w() ;
			pose.orientation.x = q.x() ;
			pose.orientation.y = q.y() ;
			pose.orientation.z = q.z() ;

			Q_EMIT moveToPose_signal(pose);
		}

		void PathPlanningWidget::on_moveToWaypointButton_clicked() {
			if (selected_waypoint_ < 0) {
				return;
			}

			sendCartTrajectoryParamsFromUI();

			Waypoint current_waypoint = getWaypointFromTreeView(selected_waypoint_);
			geometry_msgs::Pose pose_msg;
			tf::poseTFToMsg(current_waypoint.pose_, pose_msg);

			Q_EMIT moveToPose_signal(pose_msg);
		}

		void PathPlanningWidget::on_moveWaypointUpButton_clicked() {
			if (1 > selected_waypoint_) {
				return;
			}

			swapWaypoints_signal(selected_waypoint_, selected_waypoint_-1) ;

			selected_waypoint_-- ;
			ui_.treeView->selectionModel()->select (ui_.treeView->model()->index(selected_waypoint_,0), QItemSelectionModel::SelectCurrent);
		}

		void PathPlanningWidget::on_moveWaypointDownButton_clicked() {
			QAbstractItemModel *model = ui_.treeView->model();
			const int rows = model->rowCount() ;

			if (0 > selected_waypoint_ || selected_waypoint_+1 >= rows) {
				return;
			}

			swapWaypoints_signal(selected_waypoint_, selected_waypoint_+1) ;

			selected_waypoint_++ ;
			ui_.treeView->selectionModel()->select (ui_.treeView->model()->index(selected_waypoint_,0), QItemSelectionModel::SelectCurrent);
		}

		void PathPlanningWidget::setNewWaypointInputs(const tf::Transform& pose) {
			//ROS_INFO_STREAM("set new Waypoints called");

			tf::Vector3 p = pose.getOrigin();
			tfScalar rx,ry,rz;
			pose.getBasis().getRPY(rx,ry,rz,1);

			rx = RAD2DEG(rx);
			ry = RAD2DEG(ry);
			rz = RAD2DEG(rz);

			// set up the starting values for the lineEdit of the positions
			ui_.newWaypointX->setValue(p.x());
			ui_.newWaypointY->setValue(p.y());
			ui_.newWaypointZ->setValue(p.z());
			// set up the starting values for the lineEdit of the orientations, in Euler angles
			ui_.newWaypointRx->setValue(rx);
			ui_.newWaypointRy->setValue(ry);
			ui_.newWaypointRz->setValue(rz);
		}

		void PathPlanningWidget::emitNewWaypointInputValueChangedSignal() {
			ROS_INFO("Sending newWaypointInput_valueChanged signal");
			Q_EMIT newWaypointInputValueChanged(getNewWaypointInputValue());
		}

		void PathPlanningWidget::newWaypointValueChanged(double) {
			emitNewWaypointInputValueChangedSignal();
		}
	}
}
