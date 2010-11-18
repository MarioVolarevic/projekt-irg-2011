#include "windows.h"
#include "VoziloInputDeviceStateType.h"
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

#include <osg/GraphicsContext>
#include <osg/positionattitudetransform>
#include <osg/matrixtransform>
#include <osg/switch>

#include <osgGA/GUIEventHandler>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
osg::ref_ptr<osg::MatrixTransform> tran_fer = new osg::MatrixTransform();

//osg::ref_ptr<osg::Node> Model =  osgDB::readNodeFile( "../../Modeli/fermula_kork.3DS" );
//int okrenutL = 0;
//int okrenutD = 0;

#pragma region trazenje i povezivanje dijela modela
// funkcija za tra�enje dijelova modela (slu�i za animaciju kotaca)
osg::Node* FindNodeByName( osg::Node* pNode, const std::string& sName )
{
	if ( pNode->getName()==sName )
	{
		return pNode;
	}

	osg::Group* pGroup = pNode->asGroup();
	if ( pGroup )
	{
		for ( unsigned int i=0; i<pGroup->getNumChildren(); i++ )
		{
			osg::Node* pFound = FindNodeByName( pGroup->getChild(i), sName );
			if ( pFound )
			{
				return pFound;
			}
		}
	}

	return 0;
}

// funkcija za povezivanje dijela modela i transformacije
osg::MatrixTransform* AddMatrixTransform( osg::Node* pNode )
{
	// parent must derive from osg::Group

	osg::Group* pGroup = pNode->getParent(0)->asGroup();
	if ( pGroup )
	{
		// make sure we have a reference count at all time!
		osg::ref_ptr<osg::Node> pNodeTmp = pNode;

		// remove pNode from parent
		pGroup->removeChild( pNodeTmp.get() );

		// create matrixtransform and do connections
		osg::MatrixTransform* pMT = new osg::MatrixTransform;
		pMT->addChild( pNodeTmp.get() );
		pGroup->addChild( pMT );
		return pMT;
	}

	return 0;
}
#pragma endregion

class FrameLimiter
{
private:
	osg::Timer timer;
	osg::Timer_t c_time;
	osg::Timer_t p_time;
	int fps;

public:
	FrameLimiter::FrameLimiter(int n)
	{
		fps = n;
	}

	void frame_limit(){
		while(timer.delta_s(p_time,c_time) < (1.0/fps)){
			//Update the current time
			c_time = timer.tick();
		}
		//Set the previous frame time to that of the current frame time.
		p_time = c_time;
	}
};
class Vozilo
{
public:
	osg::Node* Model;
	int okrenutL;
	int okrenutD;
	Vozilo(std::string ime_mod)
	{
		Model = osgDB::readNodeFile(ime_mod);
		okrenutL = 0;
		okrenutD = 0;
	}
	//Model = osgDB::readNodeFile( "../../Modeli/fermula_kork.3DS" );

	void okreniLijevo(bool t)
	{
		if (t) okrenutL = 1;
		else okrenutD = 0;
		osg::Node* lijeviKotac = FindNodeByName( Model, "kotacPL" );
		osg::Node* desniKotac = FindNodeByName( Model, "kotacPD" );
		osg::MatrixTransform* ltr = AddMatrixTransform(lijeviKotac);
		osg::MatrixTransform* dtr = AddMatrixTransform(desniKotac);
		ltr->setMatrix(osg::Matrix::rotate(osg::inDegrees(30.0f),osg::Z_AXIS));
		dtr->setMatrix(osg::Matrix::rotate(osg::inDegrees(30.0f),osg::Z_AXIS));
	}
	void okreniDesno(bool t)
	{
		if (t) okrenutD = 1;
		else okrenutL = 0;
		osg::Node* lijeviKotac = FindNodeByName( Model, "kotacPL" );
		osg::Node* desniKotac = FindNodeByName( Model, "kotacPD" );
		osg::MatrixTransform* ltr = AddMatrixTransform(lijeviKotac);
		osg::MatrixTransform* dtr = AddMatrixTransform(desniKotac);
		ltr->setMatrix(osg::Matrix::rotate(osg::inDegrees(-30.0f),osg::Z_AXIS));
		dtr->setMatrix(osg::Matrix::rotate(osg::inDegrees(-30.0f),osg::Z_AXIS));
	}
	//void promijeniModel(std::string n)
	//{
	//	Model = osgDB::readNodeFile(n);
	//}

};
#pragma region Proximity Callback

class MarkerProximityUpdateCallback : public osg::NodeCallback 
{
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

	  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) 
	  {
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

class MyKeyboardEventHandler : public osgGA::GUIEventHandler 
{
protected:
	VoziloInputDeviceStateType* voziloInputDeviceState;
	Vozilo* v;
	//int i;

public:
	MyKeyboardEventHandler(VoziloInputDeviceStateType* vids, Vozilo* vozilo)
	{
		voziloInputDeviceState = vids;
		v = vozilo;
	}
	/**
	OVERRIDE THE HANDLE METHOD:
	The handle() method should return true if the event has been dealt with
	and we do not wish it to be handled by any other handler we may also have
	defined. Whether you return true or false depends on the behaviour you 
	want - here we have no other handlers defined so return true.
	**/
	bool MyKeyboardEventHandler::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa) 
	{

		switch (ea.getEventType()) 
		{
		case (osgGA::GUIEventAdapter::KEYDOWN): 
			{
				switch (ea.getKey()) 
				{
				case osgGA::GUIEventAdapter::KEY_Up: 
					voziloInputDeviceState->moveFwdRequest = true;
					return false;

				case osgGA::GUIEventAdapter::KEY_Left:
					{
						voziloInputDeviceState->rotLReq = true;
						if (v->okrenutL == 0)
							v->okreniLijevo(true);
						return false;
					}
				case osgGA::GUIEventAdapter::KEY_Right:
					{
						voziloInputDeviceState->rotRReq = true;
						if (v->okrenutD == 0)
							v->okreniDesno(true);
						return false;
					}
				case osgGA::GUIEventAdapter::KEY_Down:
					voziloInputDeviceState->moveBcwRequest = true;
					return false;

				case osgGA::GUIEventAdapter::KEY_F1:
					voziloInputDeviceState->resetReq = true;
					return false;

				case 'y':
					voziloInputDeviceState->promijeniModel = 1;
					return false;

				case 'x':
					voziloInputDeviceState->promijeniModel = 2;
					return false;
				default:
					return false;
				}

		case(osgGA::GUIEventAdapter::KEYUP):
			{
				switch(ea.getKey())
				{
				case osgGA::GUIEventAdapter::KEY_Up:
					voziloInputDeviceState->moveFwdRequest = false;
					return false;
				case osgGA::GUIEventAdapter::KEY_Left:
					{
						voziloInputDeviceState->rotLReq = false;
						v->okreniDesno(false);
						return false;
					}
				case osgGA::GUIEventAdapter::KEY_Right:
					{
						voziloInputDeviceState->rotRReq = false;
						v->okreniLijevo(false);
						return false;
					}
				case osgGA::GUIEventAdapter::KEY_Down:
					voziloInputDeviceState->moveBcwRequest = false;
					return false;
				case osgGA::GUIEventAdapter::KEY_F1:
					voziloInputDeviceState->resetReq = false;
					return false;
				case 'y':
					voziloInputDeviceState->promijeniModel = 0;
					return false;
				case 'x':
					voziloInputDeviceState->promijeniModel = 0;
					return false;
				default:
					return false;
				}
			}
			}
		default: return false;
		}
	}
};
#pragma endregion

class updateVoziloPosCallback: public osg::NodeCallback
{
protected:
	VoziloInputDeviceStateType* voziloInputDeviceState; 
	Vozilo* v;//za izbrisat

public:
	updateVoziloPosCallback::updateVoziloPosCallback(VoziloInputDeviceStateType* vids, Vozilo* vozilo) //izbrisat vozilo
	{
		voziloInputDeviceState = vids;
		v = vozilo; //izbrisat
	}
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	{
		osg::MatrixTransform* vmt = dynamic_cast<osg::MatrixTransform*> (node);
		
		if (vmt)
		{
			if (voziloInputDeviceState->resetReq)
			{   
				vmt->setMatrix(osg::Matrix::identity());
				vmt->preMult(osg::Matrix::translate(osg::Vec3(0,0,25)));
				vmt->preMult(osg::Matrix::scale(osg::Vec3(2,2,2)));
			}
			if (voziloInputDeviceState->moveFwdRequest)
			{
				vmt->preMult(osg::Matrix::translate(0,-2.0f,0));
			}
			if (voziloInputDeviceState->moveBcwRequest)
			{
				vmt->preMult(osg::Matrix::translate(0,0.8f,0));
			}

			if ((voziloInputDeviceState->rotLReq)&&(voziloInputDeviceState->moveFwdRequest))

			{
				vmt->preMult(osg::Matrix::rotate(osg::inDegrees(2.0f),osg::Z_AXIS));
			}
			if ((voziloInputDeviceState->rotRReq)&&(voziloInputDeviceState->moveFwdRequest))

			{
				vmt->preMult(osg::Matrix::rotate(osg::inDegrees(-2.0f),osg::Z_AXIS));
			}

			if ((voziloInputDeviceState->rotRReq)&&(voziloInputDeviceState->moveBcwRequest))

			{
				vmt->preMult(osg::Matrix::rotate(osg::inDegrees(1.2f),osg::Z_AXIS));
			}
			if ((voziloInputDeviceState->rotLReq)&&(voziloInputDeviceState->moveBcwRequest))

			{
				vmt->preMult(osg::Matrix::rotate(osg::inDegrees(-1.2f),osg::Z_AXIS));
			}
			if(voziloInputDeviceState->promijeniModel == 1) {
				tran_fer->setChild(0,v->Model = osgDB::readNodeFile("../../Modeli/ana_f1_mod.3DS")); 
			}
			else if (voziloInputDeviceState->promijeniModel == 2)
				tran_fer->setChild(0,v->Model = osgDB::readNodeFile("../../Modeli/fermula_kork.3DS")); 


		}
	}
};

#pragma region Custom Visibility Callback
//class MyMarkerVisibilityCallback : public osgART::MarkerVisibilityCallback
//{
//public:
//	MyMarkerVisibilityCallback(osgART::Marker* marker):osgART::MarkerVisibilityCallback(marker){}
//	void operator()(osg::Node* node, osg::NodeVisitor* nv) 
//	{
//		if (osg::Switch* _switch = dynamic_cast<osg::Switch*>(node)) {
//			_switch->setSingleChildOn(m_marker->valid() ? 0 : 1);   
//		} else {
//			node->setNodeMask(0xFFFFFFFF);
//			nv->setNodeMaskOverride(0x0);
//		}
//		traverse(node,nv);
//	}
//};


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
	osg::ref_ptr<osgART::Marker> markerB = tracker->addMarker("single;data/patt.sample1;80;0;0");
	if (!markerB.valid()) 
	{
		// Without marker an AR application can not work. Quit if none found.
		osg::notify(osg::FATAL) << "Could not add marker!" << std::endl;
		exit(-1);
	}
	markerB->setActive(true);

	//markerC
	osg::ref_ptr<osgART::Marker> markerC = tracker->addMarker("single;data/patt.sample2;80;0;0");
	if (!markerC.valid())
	{
		osg::notify(osg::FATAL) << "Could not add marker!" << std::endl;
		exit(-1);
	}
	markerC->setActive(true);

	//markerD
	osg::ref_ptr<osgART::Marker> markerD = tracker->addMarker("single;data/armedia.patt;75;0;0");
	if (!markerD.valid())
	{
		osg::notify(osg::FATAL) << "Could not add marker!" << std::endl;
		exit(-1);
	}
	markerD->setActive(true);

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
	arTransformB->addChild(osgDB::readNodeFile("../../Modeli/cesta_skr.3ds"));
	arTransformB->getOrCreateStateSet()->setRenderBinDetails(100,"RenderBin");

	//arTransformC
	osg::ref_ptr<osg::MatrixTransform> arTransformC = new osg::MatrixTransform();
	osgART::attachDefaultEventCallbacks(arTransformC,markerC);
	arTransformC->addChild(osgDB::readNodeFile("../../Modeli/cesta_skr.3ds"));
	arTransformC->getOrCreateStateSet()->setRenderBinDetails(100,"RenderBin");

	//arTransformD
	osg::ref_ptr<osg::MatrixTransform> arTransformD = new osg::MatrixTransform();
	osgART::attachDefaultEventCallbacks(arTransformD,markerD);
	arTransformD->addChild(osgDB::readNodeFile("../../Modeli/cesta_rav.3ds"));
	arTransformD->getOrCreateStateSet()->setRenderBinDetails(100,"RenderBin");

	//pocetno podesavanje modela
	VoziloInputDeviceStateType* vIDevState = new VoziloInputDeviceStateType;
	//osg::ref_ptr<osg::MatrixTransform> tran_fer = new osg::MatrixTransform();
	Vozilo* v1 = new Vozilo("../../Modeli/fermula_kork.3DS");
	Vozilo* v2 = new Vozilo("../../Modeli/ana_f1_mod.3DS");
	tran_fer->addChild(v1->Model);
	tran_fer->preMult(osg::Matrix::translate(osg::Vec3(0,0,25)));
	tran_fer->preMult(osg::Matrix::scale(osg::Vec3(2,2,2)));
	//update kontrola
	tran_fer->setUpdateCallback(new updateVoziloPosCallback(vIDevState,v1));
	MyKeyboardEventHandler* voziloEventHandler = new MyKeyboardEventHandler(vIDevState, v1);
	viewer.addEventHandler(voziloEventHandler);


	//grupa model i cesta
	osg::ref_ptr<osg::Group> mod_ces= new osg::Group();
	mod_ces->addChild(tran_fer);
	mod_ces->addChild(osgDB::readNodeFile("../../Modeli/cesta_rav.3ds"));


	//switch za prikazivanje dodatnog modela ovisno o udaljenosti markera
	osg::ref_ptr<osg::Switch> switchA = new osg::Switch();
	switchA->addChild(tran_fer,true);
	switchA->addChild(mod_ces,false);
	arTransformA->addChild(switchA);
	osgART::TrackerCallback::addOrSet(root.get(), tracker.get());


	cam->addChild(arTransformA);
	cam->addChild(arTransformB);
	cam->addChild(arTransformC);
	cam->addChild(arTransformD);
	cam->addChild(videoBackground.get());
	root->addChild(cam.get());
	root->setUpdateCallback(new MarkerProximityUpdateCallback(arTransformA, arTransformB,switchA,300));

	video->start();
	//int r = viewer.run();  
	FrameLimiter* fl = new FrameLimiter(60);
	viewer.realize();
	while (!viewer.done())
	{
		fl->frame_limit();
		viewer.frame();
	}
	video->close();
	//return r;
}
