#include "windows.h"

#include <osgART/Foundation>
#include <osgART/VideoLayer>
#include <osgART/PluginManager>
#include <osgART/VideoGeode>
#include <osgART/Utils>
#include <osgART/GeometryUtils>
#include <osgART/MarkerCallback>
#include <osgART/TransformFilterCallback>
#include <osgART/Marker>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osg/positionattitudetransform>
#include <osg/matrixtransform>
#include <osg/switch>

#include <osgGA/GUIEventHandler>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

osg::ref_ptr<osg::MatrixTransform> tran_fer = new osg::MatrixTransform();

#pragma region Proximity Callback

class MarkerProximityUpdateCallback : public osg::NodeCallback {

private:
	osg::MatrixTransform* mtA;
	osg::MatrixTransform* mtB;

	osg::Switch* mSwitchA;

	float mThreshold;

public:

	MarkerProximityUpdateCallback(osg::MatrixTransform* mA, osg::MatrixTransform* mB, osg::Switch* switchA, float threshold) : 
	  osg::NodeCallback(), 
		  mtA(mA), mtB(mB),
		  mSwitchA(switchA), mThreshold(threshold) { }


	  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {

		  /** CALCULATE INTER-MARKER PROXIMITY:
		  Here we obtain the current position of each marker, and the
		  distance between them by examining
		  the translation components of their parent transformation 
		  matrices **/
		  osg::Vec3 posA = mtA->getMatrix().getTrans();
		  osg::Vec3 posB = mtB->getMatrix().getTrans();
		  osg::Vec3 offset = posA - posB;
		  float distance = offset.length();
		  //scene->setUpdateCallback(new MarkerProximityUpdateCallback(mtA, mtB,switchA.get(),switchB.get(),200.0f)); 

		  /** LOAD APPROPRIATE MODELS:
		  Here we use each marker's OSG Switch node to swap between
		  models, depending on the inter-marker distance we have just 
		  calculated. **/
		  if (distance <= mThreshold) {
			  if (mSwitchA->getNumChildren() > 1) mSwitchA->setSingleChildOn(1);
		  } else {
			  if (mSwitchA->getNumChildren() > 0) mSwitchA->setSingleChildOn(0);
		  }


		  traverse(node,nv);

	  }
};
#pragma endregion

#pragma region Keyboard Handler

class MyKeyboardEventHandler : public osgGA::GUIEventHandler {

public:
	MyKeyboardEventHandler() : osgGA::GUIEventHandler() { }      
	/**
	OVERRIDE THE HANDLE METHOD:
	The handle() method should return true if the event has been dealt with
	and we do not wish it to be handled by any other handler we may also have
	defined. Whether you return true or false depends on the behaviour you 
	want - here we have no other handlers defined so return true.
	**/
	virtual bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa, 
		osg::Object* obj, osg::NodeVisitor* nv) 
	{ 
		switch (ea.getEventType()) 
		{


			/** KEY EVENTS:
			Key events have an associated key and event names.
			In this example, we are interested in keys up/down/right/left arrow
			and space bar.
			If we detect a press then we modify the transformation matrix 
			of the local transform node. **/
		case osgGA::GUIEventAdapter::KEYDOWN: 
			{

				switch (ea.getKey()) 
				{
				case osgGA::GUIEventAdapter::KEY_Up: // Move forward 5mm
					//driveCar->preMult(osg::Matrix::translate(0, -5, 0));
					tran_fer->preMult(osg::Matrix::translate(0,-5,0));
					return true;

				case osgGA::GUIEventAdapter::KEY_Down:
					tran_fer->preMult(osg::Matrix::translate(0,5,0));
					return true;

				case osgGA::GUIEventAdapter::KEY_Left:
					tran_fer->preMult(osg::Matrix::rotate(osg::DegreesToRadians(10.0f), osg::Z_AXIS));
					tran_fer->preMult(osg::Matrix::translate(0,-5,0));
					return true;

				case osgGA::GUIEventAdapter::KEY_Right:
					tran_fer->preMult(osg::Matrix::rotate(osg::DegreesToRadians(-10.0f), osg::Z_AXIS));
					tran_fer->preMult(osg::Matrix::translate(0,-5,0));
					return true;

				case osgGA::GUIEventAdapter::KEY_Page_Down:
					tran_fer->preMult(osg::Matrix::rotate(osg::DegreesToRadians(10.0f), osg::X_AXIS));   //pokret prema dolje pod kutom
					tran_fer->preMult(osg::Matrix::translate(0,-5,0));
					return true;

				case osgGA::GUIEventAdapter::KEY_Page_Up:
					tran_fer->preMult(osg::Matrix::rotate(osg::DegreesToRadians(-10.0f), osg::X_AXIS));  //pokret prema gore pod kutom
					tran_fer->preMult(osg::Matrix::translate(0,-5,0));
					return true;

				case osgGA::GUIEventAdapter::KEY_BackSpace:  //Reset transformation
					tran_fer->setMatrix(osg::Matrix::identity());
					tran_fer->preMult(osg::Matrix::translate(osg::Vec3(0,0,25)));
					tran_fer->preMult(osg::Matrix::scale(osg::Vec3(2,2,2)));
					return true;

				case ' ': // Reset the transformation
					//testirao neke stvari, zanemarite ovo, ali ostavite za svaki slucaj
					//osg::Matrix *m;
					//cesta->computeWorldToLocalMatrix(*m,nv);
					//osg::notify(osg::WARN)<<"matrica: "<<*m<<std::endl;
					//tran_fer->setMatrix(*m);
					//osg::Vec3 v;
					//arTransformA->computeLocalToWorldMatrix(*m,nv);
					//osg::notify(osg::WARN)<<"matrica m: "<<*m<<std::endl << m<<std::endl;
					//osg::notify(osg::WARN)<<"posit arTransformA: "<<arTransformA->getPosition()<<std::endl;
					//tran_fer->setPosition(arTransformA->getPosition());
					//tran_fer->setScale(osg::Vec3(2,2,2));
					//osg::notify(osg::WARN)<<"posit tran_fer: "<<tran_fer->getPosition()<<std::endl;
					//tran_fer->setAttitude(arTransformA->getMatrix
					return true;
				}

		default: return false;

			}


		}
	}
};
#pragma endregion

#pragma region Custom Visibility Callback
class MyMarkerVisibilityCallback : public osgART::MarkerVisibilityCallback
{
public:
	MyMarkerVisibilityCallback(osgART::Marker* marker):osgART::MarkerVisibilityCallback(marker){}
	void operator()(osg::Node* node, osg::NodeVisitor* nv) 
	{
		if (osg::Switch* _switch = dynamic_cast<osg::Switch*>(node)) {
			_switch->setSingleChildOn(m_marker->valid() ? 0 : 1);   
		} else {
			node->setNodeMask(0xFFFFFFFF);
			nv->setNodeMaskOverride(0x0);
		}
		traverse(node,nv);
	}
};


#pragma endregion //zasad se ne koristi

osg::Group* createImageBackground(osg::Image* video) {
	osgART::VideoLayer* _layer = new osgART::VideoLayer();
	_layer->setSize(*video);
	osgART::VideoGeode* _geode = new osgART::VideoGeode(osgART::VideoGeode::USE_TEXTURE_2D, video);
	addTexturedQuad(*_geode,video->s(),video->t());
	_layer->addChild(_geode);
	return _layer;
}



int main (int argc, char * argv[])
{
	osgViewer::Viewer viewer;

	osg::ref_ptr<osg::Group> root = new osg::Group;

	viewer.setSceneData(root.get());

	viewer.addEventHandler(new osgViewer::HelpHandler);
	viewer.addEventHandler(new osgViewer::StatsHandler);
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);
	viewer.addEventHandler(new osgViewer::ThreadingHandler);
	viewer.addEventHandler(new MyKeyboardEventHandler);


	// preload the video and tracker
	int _video_id = osgART::PluginManager::instance()->load("osgart_video_artoolkit2");
	int _tracker_id = osgART::PluginManager::instance()->load("osgart_tracker_artoolkit2");


	// Load video plugin.
	osg::ref_ptr<osgART::Video> video = dynamic_cast<osgART::Video*>(osgART::PluginManager::instance()->get(_video_id));
	// check if an instance of the video stream could be started
	if (!video.valid()) 
	{   
		// Without video an AR application can not work. Quit if none found.
		osg::notify(osg::FATAL) << "Could not initialize video plugin!" << std::endl;
		exit(-1);
	}
	// Open the video. This will not yet start the video stream but will
	// get information about the format of the video which is essential
	// for the connected tracker
	video->open();


	//Load tracker plugin
	osg::ref_ptr<osgART::Tracker> tracker = dynamic_cast<osgART::Tracker*>(osgART::PluginManager::instance()->get(_tracker_id));
	if (!tracker.valid()) 
	{
		// Without tracker an AR application can not work. Quit if none found.
		osg::notify(osg::FATAL) << "Could not initialize tracker plugin!" << std::endl;
		exit(-1);
	}
	// get the tracker calibration object
	osg::ref_ptr<osgART::Calibration> calibration = tracker->getOrCreateCalibration();
	// set the image source for the tracker


	// load camera calibration file
	if (!calibration->load(std::string("data/camera_para.dat"))) 
	{

		// the calibration file was non-existing or couldnt be loaded
		osg::notify(osg::FATAL) << "Non existing or incompatible calibration file" << std::endl;
		exit(-1);
	}
	tracker->setImage(video.get());
	//camera calibration
	osg::ref_ptr<osg::Camera> cam = calibration->createCamera();

	//markerA
	osg::ref_ptr<osgART::Marker> markerA = tracker->addMarker("single;data/patt.kanji;80;0;0");
	if (!markerA.valid()) 
	{
		// Without marker an AR application can not work. Quit if none found.
		osg::notify(osg::FATAL) << "Could not add marker!" << std::endl;
		exit(-1);
	}
	markerA->setActive(true);

	//markerB
	osg::ref_ptr<osgART::Marker> markerB = tracker->addMarker("single;data/armedia.patt;75;0;0");
	if (!markerB.valid()) 
	{
		// Without marker an AR application can not work. Quit if none found.
		osg::notify(osg::FATAL) << "Could not add marker!" << std::endl;
		exit(-1);
	}
	markerB->setActive(true);

	//video
	osg::ref_ptr<osg::Group> videoBackground = createImageBackground(video.get());
	videoBackground->getOrCreateStateSet()->setRenderBinDetails(0, "RenderBin");

	//arTransformA
	osg::ref_ptr<osg::MatrixTransform> arTransformA = new osg::MatrixTransform();
	osgART::attachDefaultEventCallbacks(arTransformA.get(),markerA.get());
	//osgART::addEventCallback(arTransformA.get(), new osgART::MarkerTransformCallback(markerA));
	//osgART::addEventCallback(arTransformA.get(), new MyMarkerVisibilityCallback(markerA));
	arTransformA->getOrCreateStateSet()->setRenderBinDetails(100, "RenderBin");

	//arTransformB
	osg::ref_ptr<osg::MatrixTransform> arTransformB = new osg::MatrixTransform();
	osgART::attachDefaultEventCallbacks(arTransformB,markerB);
	arTransformB->addChild(osgDB::readNodeFile("data/cesta_rav.3ds"));
	arTransformB->getOrCreateStateSet()->setRenderBinDetails(100,"RenderBin");

	//grupa model i cesta
	osg::ref_ptr<osg::Group> mod_ces= new osg::Group();
	mod_ces->addChild(tran_fer);
	mod_ces->addChild(osgDB::readNodeFile("data/cesta_rav.3ds"));
	
	//transformacije modela
	tran_fer->addChild(osgDB::readNodeFile("data/fermula_kork.3DS"));
	tran_fer->preMult(osg::Matrix::translate(osg::Vec3(0,0,25)));
	tran_fer->preMult(osg::Matrix::scale(osg::Vec3(2,2,2)));

	//switch za prikazivanje dodatnog modela ovisno o udaljenosti markera
	osg::ref_ptr<osg::Switch> switchA = new osg::Switch();
	switchA->addChild(osgDB::readNodeFile("data/fermula_kork.3DS"),true);
	switchA->addChild(mod_ces,false);
	arTransformA->addChild(switchA.get());
	osgART::TrackerCallback::addOrSet(root.get(), tracker.get());


	cam->addChild(arTransformA);
	cam->addChild(arTransformB);
	cam->addChild(videoBackground.get());
	root->addChild(cam.get());
	root->setUpdateCallback(new MarkerProximityUpdateCallback(arTransformA, arTransformB,switchA.get(),250));

	video->start();
	int r = viewer.run();   
	video->close();
	return r;
}
