class VoziloInputDeviceStateType
{
public:
	VoziloInputDeviceStateType::VoziloInputDeviceStateType() : 
	  moveFwdRequest(false),moveBcwRequest(false), rotLReq(false), rotRReq(false),
		  resetReq(false), promijeniModel(0){}
	  bool moveFwdRequest,moveBcwRequest,rotLReq,rotRReq,resetReq;
	  int promijeniModel;
};
