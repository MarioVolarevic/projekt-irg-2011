#include "windows.h"
#include <iostream>
#include "VoziloInputDeviceStateType.h"
#include <osgART/VideoLayer>
#include <osgART/PluginManager>
#include <osgART/GeometryUtils>
#include <osgART/MarkerCallback>
#include <osgViewer/ViewerEventHandlers>
#include <osg/ComputeBoundsVisitor>
#include <osgDB/ReadFile>

//#include <osgParticle/ExplosionEffect>
//#include <osgParticle/ExplosionDebrisEffect>
//#include <osgParticle/SmokeEffect>
//#include <osgParticle/SmokeTrailEffect>
//#include <osgParticle/FireEffect>
//#include <osgParticle/ParticleSystemUpdater>

#include <osgAudio/Source.h>
#include <osgAudio/Sample.h>
#include <osgAudio/AudioEnvironment.h>
//#include <osgART/Foundation>
//#include <osgART/VideoGeode>
//#include <osgART/Utils>
//#include <osgART/TransformFilterCallback>
//#include <osgART/Marker>
//#include <osgViewer/Viewer>
//#include <osg/GraphicsContext>
//#include <osg/positionattitudetransform>
//#include <osg/matrixtransform>
//#include <osg/switch>
//#include <osgGA/GUIEventHandler>
//#include <osgDB/FileUtils>

#define br_markera 16

float kutZakretanja=6; //default 6, kad brzina naraste na 20 onda mijenjam u 8
const float skretanjeLimit=0.4;
const int maxBrzina=40; //default 20
const int maxBrzinaR=4; //default 4
const int FrameLimit=30; //default 60
osg::ref_ptr<osgAudio::Source> zvuk = new osgAudio::Source;
std::string snd_file[9];
int play_sound = 0;
int sound_num;
bool sudar = false;


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
	double brzina;
	double vrijeme;
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

};


/************************KEYBOARD HANDLER FOR HUD*****************/
osgText::Text* brzina=new osgText::Text();
osg::Geode* HUDBrzina=new osg::Geode();
/*************************ISPIS BRZINE********************************/
void ispisBrzina(Vozilo* fermula){
	std::string brz;
	char b[5];
	itoa(fermula->brzina*2,b,10);
	brz.append(b);
	brz.append(" km/h");
	brzina->setText("Brzina: "+brz);
	return;
}
/******************************************************/
class KeyboardHandlerForHUD : public osgGA::GUIEventHandler{
public:
	KeyboardHandlerForHUD(osg::Geode* geode,osgText::Text* text,Vozilo* fermula,osg::Group* root)
		: _geode(geode),_text(text),_fermula(fermula),_root(root){}
	virtual bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&);
private:
	osg::Geode* _geode;
	osgText::Text* _text;
	Vozilo* _fermula;
	osg::Group* _root;
};

bool KeyboardHandlerForHUD::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&){
	osgText::Text* options=new osgText::Text();
	switch(ea.getEventType()){
		case(osgGA::GUIEventAdapter::KEYDOWN):{
			switch(ea.getKey()){
		case 'o':
			_geode->setNodeMask(0xffffffff - _geode->getNodeMask());
			_text->setText("Options: for more help press <p>");
			//std::cout<<_fermula->brzina;
			_text->setPosition(osg::Vec3(10,640,-1));
			return false;
		case 'p':
			_text->setText("help: \n<o> back to game\n<n> see controls\n<l> see models\n<b> show/hide speed");
			_text->setPosition(osg::Vec3(10,640,-1));
			return false;
		case 'n':
			_text->setText("Controls:\n<ARROW_UP> forward\n<ARROW_DOWN> back\n<ARROW_LEFT> left\n<ARROW_RIGHT> right\n<F1> reset\n\n<p> back to menu\n<o> back to game");
			_text->setPosition(osg::Vec3(10,640,-1));
			return false;
		case 'l':
			_text->setText("Model:\n<y> ana_f1\n<x> mrki_ferm\n<c>kork_take2\n\n<p> back to menu\n<o> back to game");
		case 'b':
			HUDBrzina->setNodeMask(0xffffffff - HUDBrzina->getNodeMask());
			return false;
		default:
			return false;
			}}
		default:
			return false;
	}
}
/********************************************/

class PlayAndSwitchSound
{
private:
	osg::Timer timer;
	osg::Timer_t start_tick;
	osg::Timer_t end_tick;
public:
	PlayAndSwitchSound(int snd)
	{
		sound_num = snd;
	}
	void psound() //sustav za mijenjanje brzina 1,2,3,4(loop)
	{
		//osg::Timer timer;
		//osg::Timer_t start_tick;
		//osg::Timer_t end_tick;
		if (play_sound == 0)
		{
			zvuk->setSound(new osgAudio::Sample(snd_file[sound_num]));
			zvuk->play();
			start_tick = timer.tick();
			play_sound = 1;
		}
		end_tick = timer.tick();
		if (sound_num == 2 && timer.delta_m(start_tick,end_tick) > 4884)
		{
			sound_num = 3;
			zvuk->setSound(new osgAudio::Sample(snd_file[sound_num]));
			zvuk->play();
		}
		if (sound_num == 3 && timer.delta_m(start_tick,end_tick) > 10552)
		{
			sound_num = 4;
			zvuk->setSound(new osgAudio::Sample(snd_file[sound_num]));
			zvuk->play();
		}
		if (sound_num == 4 && timer.delta_m(start_tick,end_tick) > 15873)
		{
			sound_num = 5;
			zvuk->setSound(new osgAudio::Sample(snd_file[sound_num]));
			zvuk->play();
		}
	}

};
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
					zvuk->setSound(new osgAudio::Sample(snd_file[0]));
					zvuk->play();
					return false;

				case 'y':
					voziloInputDeviceState->promijeniModel = 1;
					zvuk->setSound(new osgAudio::Sample(snd_file[0]));
					zvuk->play();
					return false;

				case 'x':
					voziloInputDeviceState->promijeniModel = 2;
					zvuk->setSound(new osgAudio::Sample(snd_file[0]));
					zvuk->play();
					return false;

				case 'c':
					voziloInputDeviceState->promijeniModel = 3;
					zvuk->setSound(new osgAudio::Sample(snd_file[0]));
					zvuk->play();
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
					if (play_sound==1){
						play_sound = 0;
						if(sudar == false){
							zvuk->setSound(new osgAudio::Sample(snd_file[6]));
							zvuk->play();
						}
						sound_num = 2;
						sudar = false;
					}

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
					if (play_sound==1){
						play_sound = 0;
						if(sudar == false){

							zvuk->setSound(new osgAudio::Sample(snd_file[6]));
							zvuk->play();
						}
						sound_num = 2;
						sudar = false;
					}

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

class UpdateVoziloPosCallback: public osg::NodeCallback
{
protected:
#define BR_EL_ZGRADE 5	//broj elemenata zgrade FER-a (ABC, D, gumeD, gumeL, gumeG)
	VoziloInputDeviceStateType* voziloInputDeviceState; 
	Vozilo* v;
	//bool init;
	PlayAndSwitchSound* psnd;
	osg::ComputeBoundsVisitor cbbv_granica[BR_EL_ZGRADE];
	//osg::BoundingSphere b1;
	//osg::BoundingSphere b2[BR_EL_ZGRADE];
	//osg::Node* a1;
	osg::BoundingBox boxVozilo;
	osg::BoundingBox boxZgrada;
	//float dimX1,dimX2,dimY1,dimY2,dimZ1,dimZ2;
	//float zgX1[BR_EL_ZGRADE],zgX2[BR_EL_ZGRADE],zgY1[BR_EL_ZGRADE],zgY2[BR_EL_ZGRADE],zgZ1[BR_EL_ZGRADE],zgZ2[BR_EL_ZGRADE];

public:
	UpdateVoziloPosCallback::UpdateVoziloPosCallback(VoziloInputDeviceStateType* vids, Vozilo* vozilo,osg::Node* n1)
	{
		psnd = new PlayAndSwitchSound(2);
		voziloInputDeviceState = vids;
		v = vozilo;
		//init = true;
		osg::Node* granica = FindNodeByName(n1,"abc_zgrada");
		granica->accept(cbbv_granica[0]);
		//a1 = AddMatrixTransform(abc_zg);
		//b2[0] = a1->getBound();
		granica = FindNodeByName(n1,"D_zgrada");
		granica->accept(cbbv_granica[1]);
		//a1 = AddMatrixTransform(abc_zg);
		//b2[1] = a1->getBound();
		granica = FindNodeByName(n1,"zid_D");
		granica->accept(cbbv_granica[2]);
		//a1 = AddMatrixTransform(abc_zg);
		//b2[2] = a1->getBound();
		granica = FindNodeByName(n1,"zid_L");
		granica->accept(cbbv_granica[3]);
		//a1 = AddMatrixTransform(abc_zg);
		//b2[3] = a1->getBound();
		granica = FindNodeByName(n1,"zid_G");
		granica->accept(cbbv_granica[4]);
		//a1 = AddMatrixTransform(abc_zg);
		//b2[4] = a1->getBound();
		//TO DO: Ako netko zna neka ovaj segment koda sredi tako da se parametri
		//		 omjera uèitavaju iz datoteke (one ne treba kompajlirati za svaku preinaku)
		//		 isto se odnosi i na onaj dio kod izbora vozila
		//zgX1[0]= -b2[0].radius()*0.6f;	//omjeri dimenzija abc_zgrade
		//zgX2[0]= b2[0].radius()*0.79f;
		//zgY1[0]= -b2[0].radius()*0.23f;
		//zgY2[0]= b2[0].radius()*0.25f;
		//zgZ1[0]= -b2[0].radius()*0.5f;
		//zgZ2[0]= b2[0].radius()*0.5f;
		//zgX1[1]= -b2[1].radius()*0.34f;	//omjeri dimenzija d_zgrade
		//zgX2[1]= b2[1].radius()*0.35f;
		//zgY1[1]= -b2[1].radius()*0.535f;
		//zgY2[1]= b2[1].radius()*0.7f;
		//zgZ1[1]= -b2[1].radius()*0.5f;
		//zgZ2[1]= b2[1].radius()*0.5f;
		//zgX1[2]= -b2[2].radius()*0.07f;	//omjeri dimenzija zid_D
		//zgX2[2]= b2[2].radius()*0.07f;
		//zgY1[2]= -b2[2].radius()*0.99f;
		//zgY2[2]= b2[2].radius()*0.99f;
		//zgZ1[2]= -b2[2].radius()*0.1f;
		//zgZ2[2]= b2[2].radius()*0.1f;
		//zgX1[3]= -b2[3].radius()*0.07f;	//omjeri dimenzija zid_L
		//zgX2[3]= b2[3].radius()*0.07f;
		//zgY1[3]= -b2[3].radius()*0.99f;
		//zgY2[3]= b2[3].radius()*0.99f;
		//zgZ1[3]= -b2[3].radius()*0.1f;
		//zgZ2[3]= b2[3].radius()*0.1f;
		//zgX1[4]= -b2[4].radius()*0.99f;	//omjeri dimenzija zid_G
		//zgX2[4]= b2[4].radius()*0.99f;
		//zgY1[4]= -b2[4].radius()*0.1f; 
		//zgY2[4]= b2[4].radius()*0.1f; 
		//zgZ1[4]= -b2[4].radius()*0.1f;
		//zgZ2[4]= b2[4].radius()*0.2f;

	}
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	{
		osg::MatrixTransform* vmt = dynamic_cast<osg::MatrixTransform*> (node);
		if (vmt)
		{
			if (voziloInputDeviceState->resetReq)
			{   
				vmt->setMatrix(osg::Matrix::identity());
				vmt->preMult(osg::Matrix::translate(osg::Vec3(150,-250,0)));
				v->brzina=0;
				v->vrijeme=0;
			}

			if ((voziloInputDeviceState->moveFwdRequest)&&(!voziloInputDeviceState->moveBcwRequest))
			{
				psnd->psound(); //pustaj zvuk i mijenjaj brzine

				if (v->brzina<=(maxBrzina-1)&&(v->brzina>=0))
				{

					v->brzina=(maxBrzina*(v->vrijeme)/(v->vrijeme+1));
					//v->vrijeme+=0.03;
					v->vrijeme+=0.01;

				}
				if (v->brzina<0)
				{

					v->brzina+=0.6; //pritisak na gas dok formula ide u rikverc
				}
				vmt->preMult(osg::Matrix::translate(0,-(v->brzina),0));

			}

			if ((voziloInputDeviceState->moveBcwRequest)&&(!voziloInputDeviceState->moveFwdRequest))
			{
				if(play_sound==0)
				{
					play_sound = 1;
					zvuk->setSound(new osgAudio::Sample(snd_file[7]));
					zvuk->play();	
				}
				if(v->brzina>=-maxBrzinaR)
				{
					if (v->brzina>0){
						v->brzina-=0.7f; //kocenje
						v->vrijeme=v->brzina/(maxBrzina-v->brzina);
					}
					else v->brzina-=0.2; //rikverc

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
				vmt->preMult(osg::Matrix::translate(0,-(v->brzina),0));				
			}			
			if (v->brzina>20)
				kutZakretanja = 9;
			else kutZakretanja=6;
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

			osg::ComputeBoundsVisitor cbbv; 
			vmt->accept(cbbv); 
			boxVozilo.init();
			boxVozilo = cbbv.getBoundingBox();

			if (v->brzina != 0)
			{
				kutZakretanja = 6;
				//Postavljanje dimanzija bounding boxa za svaku zgradu i provjera kolizije sa zgradama
				for(int k=0; k<BR_EL_ZGRADE; k++)
				{
					boxZgrada.init(); //Ciscenje prethodnih podataka
					boxZgrada = cbbv_granica[k].getBoundingBox();
					//boxZgrada.set(b2[k].center().x()+zgX1[k], b2[k].center().y()+zgY1[k], b2[k].center().z()+zgZ1[k],
					//	b2[k].center().x()+zgX2[k], b2[k].center().y()+zgY2[k], b2[k].center().z()+zgZ2[k]);

					if(boxVozilo.intersects(boxZgrada))
					{
						vmt->preMult(osg::Matrix::translate(0,(v -> brzina),0));
						v->brzina=0;
						v->vrijeme=0;
						kutZakretanja = 0;
						if (sudar == false){
							zvuk->setSound(new osgAudio::Sample(snd_file[8]));
							zvuk->play();
							sudar = true;
						}
					}
				}
			}
			if(voziloInputDeviceState->promijeniModel == 1) {
				vmt->setChild(0,v->Model = osgDB::readNodeFile("../../Modeli/ana_f1.IVE")); 
				voziloInputDeviceState->promijeniModel = 0;

			}
			else if (voziloInputDeviceState->promijeniModel == 2) {
				vmt->setChild(0,v->Model = osgDB::readNodeFile("../../Modeli/mrki_ferm.IVE")); 
				voziloInputDeviceState->promijeniModel = 0;

			}
			else if (voziloInputDeviceState->promijeniModel == 3) {
				vmt->setChild(0,v->Model = osgDB::readNodeFile("../../Modeli/kork_take2.IVE")); 
				voziloInputDeviceState->promijeniModel = 0;

			}
		}
		ispisBrzina(v);
	}
};

osg::Group* createImageBackground(osg::Image* video) {
	osgART::VideoLayer* _layer = new osgART::VideoLayer();
	_layer->setSize(*video);
	osgART::VideoGeode* _geode = new osgART::VideoGeode(osgART::VideoGeode::USE_TEXTURE_2D, video);
	addTexturedQuad(*_geode,video->s(),video->t());
	_layer->addChild(_geode);
	return _layer;
}

//class EmitterUpdateCallback : public osg::NodeCallback {
//
//private:
//
//	osgParticle::PointPlacer* placer;
//	osg::MatrixTransform* trans;
//
//public:
//
//	EmitterUpdateCallback(osgParticle::PointPlacer* p, osg::MatrixTransform* mt) {
//		placer = p;
//		trans = mt;
//	}
//
//	void operator() (osg::Node* node, osg::NodeVisitor* nv) {
//		placer->setCenter(trans->getMatrix().getTrans());
//		traverse(node,nv);
//	}
//};
//
//class SmokeParticleSystem : public osg::Group {
//private:
//	osgParticle::PointPlacer* placer;	
//public:
//	SmokeParticleSystem(osg::Group* scene, osg::MatrixTransform*mt)
//	{
//		osgParticle::ParticleSystem* dustParticleSystem = new osgParticle::ParticleSystem;
//		osg::Geode *geode = new osg::Geode();
//		geode->addDrawable(dustParticleSystem);
//		this->addChild(geode);
//		dustParticleSystem->setDefaultAttributes("smoke.rgb",false,false,0);
//		osgParticle::Particle smokeParticle;
//		smokeParticle.setSizeRange(osgParticle::rangef(4.0f,20.0f));
//		smokeParticle.setLifeTime(15);
//		smokeParticle.setMass(0.4f);
//		smokeParticle.setColorRange(osgParticle::rangev4(osg::Vec4(1,1,1,1),osg::Vec4(0,0,0,0.3f)));
//		dustParticleSystem->setDefaultParticleTemplate(smokeParticle);
//		osgParticle::ParticleSystemUpdater *dustSystemUpdater = new osgParticle::ParticleSystemUpdater;
//		dustSystemUpdater->addParticleSystem(dustParticleSystem);
//		this->addChild(dustSystemUpdater);
//		osgParticle::ModularEmitter *emitter = new osgParticle::ModularEmitter;
//		emitter->setParticleSystem(dustParticleSystem);
//		this->addChild(emitter);
//		osgParticle::RandomRateCounter *dustRate = static_cast<osgParticle::RandomRateCounter*> (emitter->getCounter());
//		dustRate->setRateRange(40,80);
//		osgParticle::PointPlacer* placer = 
//			static_cast<osgParticle::PointPlacer *>(emitter->getPlacer());
//		this->setUpdateCallback(new EmitterUpdateCallback(placer, mt));
//	}
//};
//
int main (int argc, char * argv[])
{
	osgViewer::Viewer viewer;

	osg::ref_ptr<osg::Group> root = new osg::Group;

	viewer.setSceneData(root.get());

	viewer.addEventHandler(new osgViewer::HelpHandler);
	viewer.addEventHandler(new osgViewer::StatsHandler);
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);
	viewer.addEventHandler(new osgViewer::ThreadingHandler);

	//sound init
	osgAudio::AudioEnvironment::instance()->init();
	//std::string snd_file[6];
	snd_file[0] = "start_l.wav";
	snd_file[1] = "idle_l.wav";
	snd_file[2] = "1first.wav";
	snd_file[3] = "2second.wav";
	snd_file[4] = "3third.wav";
	snd_file[5] = "4fourth_cruise_l.wav";
	snd_file[6] = "off_fast_l.wav";
	snd_file[7] = "Reverse_l.wav";
	snd_file[8] = "sudar.wav";
	//snd_file[9] = "idle_l.wav";
	zvuk->setSound(new osgAudio::Sample(snd_file[0]));
	zvuk->play();
	zvuk->setLooping(true);
	//zvuk->setGain(0.6f);

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
	if (!calibration->load(std::string("data/log_calib_para.dat"))) 
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
	marker[0] = tracker->addMarker("single;data/ca.p;60;0;0");
	marker[1] = tracker->addMarker("single;data/cb.p;60;0;0");
	marker[2] = tracker->addMarker("single;data/cc.p;60;0;0");
	marker[3] = tracker->addMarker("single;data/cd.p;60;0;0");
	marker[4] = tracker->addMarker("single;data/ce.p;60;0;0");
	marker[5] = tracker->addMarker("single;data/za.p;60;0;0");
	marker[6] = tracker->addMarker("single;data/zb.p;60;0;0");
	marker[7] = tracker->addMarker("single;data/zc.p;60;0;0");
	marker[8] = tracker->addMarker("single;data/zd.p;60;0;0");
	marker[9] = tracker->addMarker("single;data/ze.p;60;0;0");
	marker[10] = tracker->addMarker("single;data/oa.p;60;0;0");
	marker[11] = tracker->addMarker("single;data/ob.p;60;0;0");
	marker[12] = tracker->addMarker("single;data/oc.p;60;0;0");
	marker[13] = tracker->addMarker("single;data/od.p;60;0;0");
	marker[14] = tracker->addMarker("single;data/oe.p;60;0;0");
	marker[15] = tracker->addMarker("single;data/of.p;60;0;0");
	//aktiviranje markera
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
	osg::ref_ptr<osg::MatrixTransform> dim = new osg::MatrixTransform();
	for (int i = 0; i < br_markera; i++)
	{
		arT[i] = new osg::MatrixTransform();
		arT[i]->getOrCreateStateSet()->setRenderBinDetails(300, "RenderBin");
		osgART::attachDefaultEventCallbacks(arT[i],marker[i]);
	}
	for (int i = 0; i<5; i++){
		arT[i]->addChild(osgDB::readNodeFile("../../Modeli/cesta_rav.IVE"));
		arT[i+5]->addChild(osgDB::readNodeFile("../../Modeli/cesta_skr.IVE"));}


	arT[10]->addChild(osgDB::readNodeFile("../../Modeli/reklama.IVE"));
	arT[11]->addChild(osgDB::readNodeFile("../../Modeli/snjegovic.IVE"));
	arT[12]->addChild(osgDB::readNodeFile("../../Modeli/klupa_drvo.IVE"));
	arT[13]->addChild(osgDB::readNodeFile("../../Modeli/rekl3.IVE"));
	arT[14]->addChild(osgDB::readNodeFile("../../Modeli/rekl2.IVE"));
	arT[15]->addChild(osgDB::readNodeFile("../../Modeli/kuca_drvo.IVE"));

	////particle system
	////osg::ref_ptr<SmokeParticleSystem> smoke = new SmokeParticleSystem(root,arT[15]);
	//osg::ref_ptr<SmokeParticleSystem> smoke = new SmokeParticleSystem(root,arT[15]);
	//arT[15]->addChild(dim);
	//dim->addChild(smoke);
	//dim->preMult(osg::Matrix::translate(osg::Vec3(0,0,335)));
	//dim->getOrCreateStateSet()->setRenderBinDetails(150,"RenderBin");

	//arT[15]->addChild(smoke);

	//arTMulti
	osg::ref_ptr<osg::MatrixTransform> multiTrans = new osg::MatrixTransform();
	osgART::attachDefaultEventCallbacks(multiTrans, markerMult);
	osg::ref_ptr<osg::MatrixTransform> zg_fer = new osg::MatrixTransform();
	zg_fer->getOrCreateStateSet()->setRenderBinDetails(400,"RenderBin");
	zg_fer->addChild(osgDB::readNodeFile("../../Modeli/abcd_zg_gume_spojeno_smotra.IVE"));
	multiTrans->addChild(zg_fer);
	osg::ref_ptr<osg::MatrixTransform> tlo = new osg::MatrixTransform();
	tlo->addChild(osgDB::readNodeFile("../../Modeli/tlo.IVE"));
	tlo->getOrCreateStateSet()->setRenderBinDetails(50,"RenderBin");
	tlo->preMult(osg::Matrix::translate(osg::Vec3(0,0,-30)));
	multiTrans->addChild(tlo);
	//multiTrans->getOrCreateStateSet()->setRenderBinDetails(50,"RenderBin");

	//pocetno podesavanje modela
	VoziloInputDeviceStateType* vIDevState = new VoziloInputDeviceStateType;
	Vozilo* v = new Vozilo("../../Modeli/ana_f1.IVE");
	osg::ref_ptr<osg::MatrixTransform> tran_fermula = new osg::MatrixTransform();
	tran_fermula->addChild(v->Model);
	tran_fermula->preMult(osg::Matrix::translate(osg::Vec3(150,-250,0)));
	tran_fermula->getOrCreateStateSet()->setRenderBinDetails(450,"RenderBin");
	//update kontrola
	tran_fermula->setUpdateCallback(new UpdateVoziloPosCallback(vIDevState,v,zg_fer));
	MyKeyboardEventHandler* voziloEventHandler = new MyKeyboardEventHandler(vIDevState, v);
	viewer.addEventHandler(voziloEventHandler);
	multiTrans->addChild(tran_fermula);
	osg::ref_ptr<osg::MatrixTransform> tr_ces = new osg::MatrixTransform(osg::Matrix::translate(osg::Vec3(0,-170,0)));

	osgART::TrackerCallback::addOrSet(root.get(), tracker.get());


#pragma region HUD
	/************HUD**************/
	osg::Geode* HUDGeode = new osg::Geode();
	osgText::Text* HUDText = new osgText::Text();
	osg::Projection* HUDProjectionMatrix = new osg::Projection;


	HUDProjectionMatrix->setMatrix(osg::Matrix::ortho2D(0,1024,0,768));
	osg::MatrixTransform* HUDModelViewMatrix = new osg::MatrixTransform;
	HUDModelViewMatrix->setMatrix(osg::Matrix::identity());
	HUDModelViewMatrix->setReferenceFrame(osg::Transform::ABSOLUTE_RF);

	root->addChild(HUDProjectionMatrix);
	HUDProjectionMatrix->addChild(HUDModelViewMatrix);
	HUDModelViewMatrix->addChild(HUDGeode);
	osg::Geometry* HUDGeometry = new osg::Geometry();

	osg::Vec3Array* HUDVertices = new osg::Vec3Array;
	HUDVertices->push_back(osg::Vec3(0,100,-1));
	HUDVertices->push_back(osg::Vec3(1024,100,-1));
	HUDVertices->push_back(osg::Vec3(1024,768,-1));
	HUDVertices->push_back(osg::Vec3(0,768,-1));

	osg::DrawElementsUInt* HUDIndices= new osg::DrawElementsUInt(osg::PrimitiveSet::POLYGON,0);
	HUDIndices->push_back(0);
	HUDIndices->push_back(1);
	HUDIndices->push_back(2);
	HUDIndices->push_back(3);

	osg::Vec4Array* HUDColors = new osg::Vec4Array;
	HUDColors->push_back(osg::Vec4(0.8f,0.8f,0.8f,0.8f));

	osg::Vec2Array* textcoords = new osg::Vec2Array(4);
	(*textcoords)[0].set(0.0f,0.0f);
	(*textcoords)[1].set(1.0f,0.0f);
	(*textcoords)[2].set(1.0f,1.0f);
	(*textcoords)[3].set(0.0f,1.0f);

	HUDGeometry->setTexCoordArray(0,textcoords);
	osg::Texture2D* HUDTexture = new osg::Texture2D;
	HUDTexture->setDataVariance(osg::Object::DYNAMIC);
	//osg::Image* HUDImage;
	//HUDImage=osgDB::readImageFile("grass.bmp");
	//HUDTexture->setImage(HUDImage);
	osg::Vec3Array* HUDnormals= new osg::Vec3Array;
	HUDnormals->push_back(osg::Vec3(0.0f,0.0f,1.0f));
	HUDGeometry->setNormalArray(HUDnormals);
	HUDGeometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
	HUDGeometry->addPrimitiveSet(HUDIndices);
	HUDGeometry->setVertexArray(HUDVertices);
	HUDGeometry->setColorArray(HUDColors);
	HUDGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

	HUDGeode->addDrawable(HUDGeometry);

	HUDGeode->addDrawable(HUDText);

	osg::StateSet* HUDStateSet = new osg::StateSet();
	HUDGeode->setStateSet(HUDStateSet);
	HUDStateSet->setTextureAttribute(0,HUDTexture,osg::StateAttribute::ON);
	HUDStateSet->setMode(GL_BLEND,osg::StateAttribute::ON);
	HUDStateSet->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
	HUDStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
	HUDStateSet->setRenderBinDetails(500,"RenderBin");


	HUDText->setCharacterSize(25);
	HUDText->setFont("sfdr.ttf");
	HUDText->setText("");
	//HUDText->setAlignment(osgText::Text::CENTER_TOP);
	HUDText->setAxisAlignment(osgText::Text::SCREEN);
	HUDText->setPosition(osg::Vec3(450,740,-1));
	HUDText->setColor(osg::Vec4(1,0,0,1));

	HUDModelViewMatrix->addChild(HUDBrzina);
	brzina->setColor(osg::Vec4(1,0,0,1));
	brzina->setCharacterSize(35);
	brzina->setFont("sfdr.ttf");
	brzina->setPosition(osg::Vec3(800,740,-1));
	HUDBrzina->addDrawable(brzina);
	HUDGeode->setNodeMask(0);
	ispisBrzina(v);
	/**********************************/
	viewer.addEventHandler(new KeyboardHandlerForHUD(HUDGeode,HUDText,v,root));
#pragma endregion

	for (int i = 0; i<br_markera;i++)
		cam->addChild(arT[i]);

	cam->addChild(multiTrans);
	cam->addChild(videoBackground.get());
	root->addChild(cam.get());
	video->start();
	FrameLimiter* fl = new FrameLimiter(FrameLimit); 
	viewer.realize();
	while (!viewer.done())
	{
		fl->frame_limit();
		viewer.frame();
	}
	video->close();
}