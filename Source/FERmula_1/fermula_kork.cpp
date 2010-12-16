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

#define br_markera 4

const float kutZakretanja=6;
const float skretanjeLimit=0.4;
const int maxBrzina=20;
const int maxBrzinaR=4;

//osg::ref_ptr<osg::MatrixTransform> tran_fer = new osg::MatrixTransform();

//osg::ref_ptr<osg::Node> Model =  osgDB::readNodeFile( "../../Modeli/fermula_kork.3DS" );
//int okrenutL = 0;
//int okrenutD = 0;

#pragma region trazenje i povezivanje dijela modela
// funkcija za traženje dijelova modela (služi za animaciju kotaca)
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
	float brzina;
	float vrijeme;
	int okrenutL;
	int okrenutD;
	Vozilo(std::string ime_mod)
	{
		Model = osgDB::readNodeFile(ime_mod);
		okrenutL = 0;
		okrenutD = 0;
		brzina = 0.f;
		vrijeme = 0.f;
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
//
//class MarkerProximityUpdateCallback : public osg::NodeCallback 
//{
//private:
//	osg::MatrixTransform* mtA;
//	osg::MatrixTransform* mtB;
//	osg::Switch* mSwitchA;
//	float mThreshold;
//
//public:
//	MarkerProximityUpdateCallback(osg::MatrixTransform* mA, osg::MatrixTransform* mB, osg::Switch* switchA, float threshold) : 
//	  osg::NodeCallback(), 
//		  mtA(mA), mtB(mB),
//		  mSwitchA(switchA), mThreshold(threshold) { }
//
//	  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) 
//	  {
//		  /** CALCULATE INTER-MARKER PROXIMITY:
//		  Here we obtain the current position of each marker, and the
//		  distance between them by examining
//		  the translation components of their parent transformation 
//		  matrices **/
//		  osg::Vec3 posA = mtA->getMatrix().getTrans();
//		  osg::Vec3 posB = mtB->getMatrix().getTrans();
//		  osg::Vec3 offset = posA - posB;
//		  float distance = offset.length();
//		  //scene->setUpdateCallback(new MarkerProximityUpdateCallback(mtA, mtB,switchA.get(),switchB.get(),200.0f)); 
//
//		  /** LOAD APPROPRIATE MODELS:
//		  Here we use each marker's OSG Switch node to swap between
//		  models, depending on the inter-marker distance we have just 
//		  calculated. **/
//		  if (distance <= mThreshold) {
//			  if (mSwitchA->getNumChildren() > 1) mSwitchA->setSingleChildOn(1);
//		  } else {
//			  if (mSwitchA->getNumChildren() > 0) mSwitchA->setSingleChildOn(0);
//		  }
//		  traverse(node,nv);
//	  }
//};
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

				case 'c':
					voziloInputDeviceState->promijeniModel = 3;
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
				case 'c':
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

//class CollisionTestCallback : public osg::NodeCallback
//{
//protected:
//osg::BoundingSphere b1, b2;
//	osg::Node*a1,*a2;
//public:
//	CollisionTestCallback::CollisionTestCallback(osg::Node* n1,osg::Node* n2)
//	{
//		a1=n1;
//		a2=n2;
//		b1 = a1->getBound();
//		b2 = a2->getBound();
//	}
//	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
//	{
//		if(b1.intersects(b2)) std::cout << "Collision" << std::endl;
//		else std::cout << "No Collision" << std::endl;
//		//float r1 = b1.radius();		
//		//float r2 = b2.radius();
//		//osg::Vec3 c1 = b1.center();
//		//osg::Vec3 c2 = b2.center();
//		//std::cout <<"radius1 = "<< r1 <<" centar1 = "<< c1 <<std::endl;
//		//std::cout <<"radius2 = "<< r2 <<" centar2 = "<< c2 <<std::endl;
//	}
//};

class UpdateVoziloPosCallback: public osg::NodeCallback
{
protected:
	VoziloInputDeviceStateType* voziloInputDeviceState; 
	Vozilo* v;
#define BR_EL_ZGRADE 5	//broj elemenata zgrade FER-a (ABC, D, gumeD, gumeL, gumeG)
	osg::BoundingSphere b1;
	osg::BoundingSphere b2[BR_EL_ZGRADE];
	osg::Node* a1;
	osg::BoundingBox boxVozilo;
	osg::BoundingBox boxZgrada;
	float dimX1,dimX2,dimY1,dimY2,dimZ1,dimZ2;
	float zgX1[BR_EL_ZGRADE],zgX2[BR_EL_ZGRADE],zgY1[BR_EL_ZGRADE],zgY2[BR_EL_ZGRADE],zgZ1[BR_EL_ZGRADE],zgZ2[BR_EL_ZGRADE];

public:
	UpdateVoziloPosCallback::UpdateVoziloPosCallback(VoziloInputDeviceStateType* vids, Vozilo* vozilo,osg::Node* n1)
	{
		voziloInputDeviceState = vids;
		v = vozilo;

		osg::Node* abc_zg = FindNodeByName(n1,"abc_zgrada");
		a1 = AddMatrixTransform(abc_zg);
		b2[0] = a1->getBound();
		abc_zg = FindNodeByName(n1,"D_zgrada");
		a1 = AddMatrixTransform(abc_zg);
		b2[1] = a1->getBound();
		abc_zg = FindNodeByName(n1,"zid_D");
		a1 = AddMatrixTransform(abc_zg);
		b2[2] = a1->getBound();
		abc_zg = FindNodeByName(n1,"zid_L");
		a1 = AddMatrixTransform(abc_zg);
		b2[3] = a1->getBound();
		abc_zg = FindNodeByName(n1,"zid_G");
		a1 = AddMatrixTransform(abc_zg);
		b2[4] = a1->getBound();

		//TO DO: Ako netko zna neka ovaj segment koda sredi tako da se parametri
		//		 omjera uèitavaju iz datoteke (one ne treba kompajlirati za svaku preinaku)
		//		 isto se odnosi i na onaj dio kod izbora vozila
		zgX1[0]= -b2[0].radius()*0.6f;	//omjeri dimenzija abc_zgrade
		zgX2[0]= b2[0].radius()*0.79f;
		zgY1[0]= -b2[0].radius()*0.23f;
		zgY2[0]= b2[0].radius()*0.23f;
		zgZ1[0]= -b2[0].radius()*0.5f;
		zgZ2[0]= b2[0].radius()*0.5f;
		zgX1[1]= -b2[1].radius()*0.34f;	//omjeri dimenzija d_zgrade
		zgX2[1]= b2[1].radius()*0.35f;
		zgY1[1]= -b2[1].radius()*0.38f;
		zgY2[1]= b2[1].radius()*0.5f;
		zgZ1[1]= -b2[1].radius()*0.5f;
		zgZ2[1]= b2[1].radius()*0.5f;
		zgX1[2]= -b2[2].radius()*0.07f;	//omjeri dimenzija zid_D
		zgX2[2]= b2[2].radius()*0.07f;
		zgY1[2]= -b2[2].radius()*0.99f;
		zgY2[2]= b2[2].radius()*0.99f;
		zgZ1[2]= -b2[2].radius()*0.1f;
		zgZ2[2]= b2[2].radius()*0.1f;
		zgX1[3]= -b2[3].radius()*0.07f;	//omjeri dimenzija zid_L
		zgX2[3]= b2[3].radius()*0.07f;
		zgY1[3]= -b2[3].radius()*0.99f;
		zgY2[3]= b2[3].radius()*0.99f;
		zgZ1[3]= -b2[3].radius()*0.1f;
		zgZ2[3]= b2[3].radius()*0.1f;
		zgX1[4]= -b2[4].radius()*0.99f;	//omjeri dimenzija zid_G
		zgX2[4]= b2[4].radius()*0.99f;
		zgY1[4]= -b2[4].radius()*0.05f;
		zgY2[4]= -b2[4].radius()*0.01f;
		zgZ1[4]= -b2[4].radius()*0.1f;
		zgZ2[4]= b2[4].radius()*0.1f;
	}
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	{
		osg::MatrixTransform* vmt = dynamic_cast<osg::MatrixTransform*> (node);
		if (vmt)
		{
			if (voziloInputDeviceState->resetReq)
			{   
				vmt->setMatrix(osg::Matrix::identity());
				vmt->preMult(osg::Matrix::translate(osg::Vec3(0,-170,10)));
				v->brzina=0;
				v->vrijeme=0;
			}
			if ((voziloInputDeviceState->moveFwdRequest)&&(!voziloInputDeviceState->moveBcwRequest))

			{
				if (v->brzina<=(maxBrzina-1)&&(v->brzina>=0))

				{
					
					v->brzina=(maxBrzina*(v->vrijeme)/(v->vrijeme+1));
					v->vrijeme+=0.03f;

					/*if (v->brzina>v->brzina2) { //dodaj brzina2
					v->brzina2+=1;
					std::cout << "brzinaF = " << v -> brzina << std::endl;
					std::cout << "brzinaSfF time  = " << v -> vrijeme << std::endl;
					}*/
				}
				if (v->brzina<0)
				{
					v->brzina+=0.6; //pritisak na gas dok formula ide u rikverc
				}
				vmt->preMult(osg::Matrix::translate(0,-(v->brzina),0));

			}

			if ((voziloInputDeviceState->moveBcwRequest)&&(!voziloInputDeviceState->moveFwdRequest))

			{
				if(v->brzina>=-maxBrzinaR)
				{
					if (v->brzina>0){
						v->brzina-=0.7f; //kocenje
						v->vrijeme=v->brzina/(maxBrzina-v->brzina);
					}
					else v->brzina-=0.2; //rikverc
                       
					std::cout << "brzinaB = " << v -> brzina << std::endl;
		
				}
				vmt->preMult(osg::Matrix::translate(0,-(v -> brzina),0));
			} 
			if (((!voziloInputDeviceState->moveBcwRequest)&&(!voziloInputDeviceState->moveFwdRequest))
				||(voziloInputDeviceState->moveBcwRequest)&&(voziloInputDeviceState->moveFwdRequest))
			{
				if (v->brzina > 0){
					v->brzina-=0.25f;
					if (v->brzina<0.03) v->brzina=0;
					v->vrijeme=v->brzina/(maxBrzina-v->brzina);
				}
					
				if (v->brzina < 0){
					v->brzina+=0.25f;
				}
				std::cout << "brzinaSfF = " << v -> brzina << std::endl;
				std::cout << "brzinaSfF time  = " << v -> vrijeme << std::endl;
					vmt->preMult(osg::Matrix::translate(0,-(v->brzina),0));				
			}			
			if ((voziloInputDeviceState->rotLReq)&&(v->brzina>skretanjeLimit))
			{
				vmt->preMult(osg::Matrix::rotate(osg::inDegrees(kutZakretanja),osg::Z_AXIS));
			}
			if ((voziloInputDeviceState->rotRReq)&&(v->brzina>skretanjeLimit))
			{
				vmt->preMult(osg::Matrix::rotate(osg::inDegrees(-kutZakretanja),osg::Z_AXIS));
			}

			if ((voziloInputDeviceState->rotRReq)&&(v->brzina<-skretanjeLimit))
			{
				vmt->preMult(osg::Matrix::rotate(osg::inDegrees(kutZakretanja),osg::Z_AXIS));
			}
			if ((voziloInputDeviceState->rotLReq)&&(v->brzina<-skretanjeLimit))
			{
				vmt->preMult(osg::Matrix::rotate(osg::inDegrees(-kutZakretanja),osg::Z_AXIS));
			}
					
			//Detekcija kolizije, koristi kvadre
			b1=vmt->getBound();
			//std::cout << "Centar vozila: " << b1.center().x() << "," << b1.center().y() << "," << b1.center().z() << "  Radius vozila: " << b1.radius();
			//std::cout << "Centar zgrade: " << b2.center().x() << "," << b2.center().y() << "," << b2.center().z() << "  Radius zgrade: " << b2.radius();
			//Postavljanje dimanzija bounding boxa vozila
			boxVozilo.init(); //Èišæenje prethodnih podataka
			boxVozilo.set(b1.center().x()+dimX1, b1.center().y()+dimY1, b1.center().z()+dimZ1,
				b1.center().x()+dimX2, b1.center().y()+dimY2, b1.center().z()+dimZ2);  //skaliranje kugle u kvadar
			//Postavljanje dimanzija bounding boxa za svaku zgradu i provjera kolizije sa zgradama
			for(int k=0; k<BR_EL_ZGRADE; k++)
			{
				boxZgrada.init(); //Èišæenje prethodnih podataka
				boxZgrada.set(b2[k].center().x()+zgX1[k], b2[k].center().y()+zgY1[k], b2[k].center().z()+zgZ1[k],
				b2[k].center().x()+zgX2[k], b2[k].center().y()+zgY2[k], b2[k].center().z()+zgZ2[k]);

				if(boxVozilo.intersects(boxZgrada))
				{
					std::cout << "Collision" << std::endl;

					vmt->preMult(osg::Matrix::translate(0,(v -> brzina),0));
					vmt->preMult(osg::Matrix::rotate(osg::inDegrees(-1.2f),osg::Z_AXIS));
					v->brzina=0;
				}
			}

			if(voziloInputDeviceState->promijeniModel == 1) {
				vmt->setChild(0,v->Model = osgDB::readNodeFile("../../Modeli/ana_f1.IVE")); 
				voziloInputDeviceState->promijeniModel = 0;
				dimX1= -vmt->getBound().radius()*0.42f; //omjeri dimenzija vozila
				dimX2= vmt->getBound().radius()*0.42f;
				dimY1= -vmt->getBound().radius()*1.95f;
				dimY2= vmt->getBound().radius()*1.95f;
				dimZ1= -vmt->getBound().radius()*0.333f;
				dimZ2= vmt->getBound().radius()*0.333f;
			}
			else if (voziloInputDeviceState->promijeniModel == 2) {
				vmt->setChild(0,v->Model = osgDB::readNodeFile("../../Modeli/mrki_ferm.IVE")); 
				voziloInputDeviceState->promijeniModel = 0;
				dimX1= -vmt->getBound().radius()*0.45f;
				dimX2= vmt->getBound().radius()*0.45f;
				dimY1= -vmt->getBound().radius()*1.93f;
				dimY2= vmt->getBound().radius()*1.93f;
				dimZ1= -vmt->getBound().radius()*0.333f;
				dimZ2= vmt->getBound().radius()*0.333f;
			}
			else if (voziloInputDeviceState->promijeniModel == 3) {
				vmt->setChild(0,v->Model = osgDB::readNodeFile("../../Modeli/kork_take2.IVE")); 
				voziloInputDeviceState->promijeniModel = 0;
				dimX1= -vmt->getBound().radius()*0.45f;
				dimX2= vmt->getBound().radius()*0.45f;
				dimY1= -vmt->getBound().radius()*0.90f;
				dimY2= vmt->getBound().radius()*0.93f;
				dimZ1= -vmt->getBound().radius()*0.333f;
				dimZ2= vmt->getBound().radius()*0.333f;
			}

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

	//polje markera
	osg::ref_ptr<osgART::Marker> marker[br_markera];
	//markerA
	marker[0] = tracker->addMarker("single;data/patt.kanji;80;0;0");
	//markerB
	marker[1] = tracker->addMarker("single;data/patt.sample1;80;0;0");
	//markerC
	marker[2]= tracker->addMarker("single;data/patt.sample2;80;0;0");
	//markerD
	marker[3] = tracker->addMarker("single;data/armedia.patt;10;0;0");
	for (int i=0;i<br_markera; i++)
		marker[i]->setActive(true);

	//multimarker: zgrada fera i trava
	osg::ref_ptr<osgART::Marker> markerMult = tracker->addMarker("multi;data/multi/marker_list.dat;60;0;0");
	if (!(markerMult.valid()))
	{
		osg::notify(osg::FATAL) << "Could not add marker! multi" << std::endl;
		exit(-1);
	}
	markerMult->setActive(true);

	//video
	osg::ref_ptr<osg::Group> videoBackground = createImageBackground(video.get());
	videoBackground->getOrCreateStateSet()->setRenderBinDetails(0, "RenderBin");

	//inicijalizacija transformacija i povezivanje s markerom
	osg::ref_ptr<osg::MatrixTransform> arT[br_markera];
	for (int i = 0; i < br_markera; i++)
	{
		arT[i] = new osg::MatrixTransform();
		osgART::attachDefaultEventCallbacks(arT[i],marker[i]);
		arT[i]->getOrCreateStateSet()->setRenderBinDetails(100, "RenderBin");
	}

	//arTA, kanjii
	arT[0]->addChild(osgDB::readNodeFile("../../Modeli/snjegovic.IVE"));
	//osgART::addEventCallback(arTA.get(), new osgART::MarkerTransformCallback(markerA));
	//osgART::addEventCallback(arTA.get(), new MyMarkerVisibilityCallback(markerA));
	//arTB, sample1
	arT[1]->addChild(osgDB::readNodeFile("../../Modeli/reklama.IVE"));
	//arTC, sample2
	arT[2]->addChild(osgDB::readNodeFile("../../Modeli/kuca_drvo.IVE"));
	//arTD, armedia
	arT[3]->addChild(osgDB::readNodeFile("../../Modeli/klupa_drvo.IVE"));

	//arTMulti
	osg::ref_ptr<osg::MatrixTransform> multiTrans = new osg::MatrixTransform();
	osgART::attachDefaultEventCallbacks(multiTrans, markerMult);
	osg::ref_ptr<osg::MatrixTransform> zg_fer = new osg::MatrixTransform();
	//zg_fer->addChild(osgDB::readNodeFile("../../Modeli/fer_zgrada_scale.3DS"));
	zg_fer->addChild(osgDB::readNodeFile("../../Modeli/abcd_zg_gume_spojeno.IVE"));
	multiTrans->addChild(zg_fer);
	multiTrans->getOrCreateStateSet()->setRenderBinDetails(100,"RenderBin");

	//pocetno podesavanje modela
	VoziloInputDeviceStateType* vIDevState = new VoziloInputDeviceStateType;
	Vozilo* v = new Vozilo("../../Modeli/ana_f1.IVE");
	osg::ref_ptr<osg::MatrixTransform> tran_fer = new osg::MatrixTransform();
	tran_fer->addChild(v->Model);
	tran_fer->preMult(osg::Matrix::translate(osg::Vec3(0,-170,10)));
	//update kontrola
	//tran_fer->setUpdateCallback(new UpdateVoziloPosCallback(vIDevState,v));
	tran_fer->setUpdateCallback(new UpdateVoziloPosCallback(vIDevState,v,zg_fer));
	MyKeyboardEventHandler* voziloEventHandler = new MyKeyboardEventHandler(vIDevState, v);
	viewer.addEventHandler(voziloEventHandler);
	multiTrans->addChild(tran_fer);
	osg::ref_ptr<osg::MatrixTransform> tr_ces = new osg::MatrixTransform(osg::Matrix::translate(osg::Vec3(0,-170,0)));
	tr_ces->preMult(osg::Matrix::scale(osg::Vec3(1,0.5,1)));
	tr_ces->addChild(osgDB::readNodeFile("../../Modeli/cesta_rav.3ds"));
	multiTrans->addChild(tr_ces);
	////grupa model i cesta
	//osg::ref_ptr<osg::Group> mod_ces= new osg::Group();
	//mod_ces->addChild(tran_fer);
	//mod_ces->addChild(osgDB::readNodeFile("../../Modeli/cesta_rav.3ds"));

	////switch za prikazivanje dodatnog modela ovisno o udaljenosti markera
	//osg::ref_ptr<osg::Switch> switchA = new osg::Switch();
	//switchA->addChild(tran_fer,true);
	//switchA->addChild(mod_ces,false);
	//arTA->addChild(switchA);

	osgART::TrackerCallback::addOrSet(root.get(), tracker.get());

	//cam->setUpdateCallback(new CollisionTestCallback(tran_fer, zg_fer));

	for (int i = 0; i<br_markera;i++)
		cam->addChild(arT[i]);

	cam->addChild(multiTrans);
	cam->addChild(videoBackground.get());
	root->addChild(cam.get());
	//root->setUpdateCallback(new MarkerProximityUpdateCallback(arTA, arTB,switchA,300));
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