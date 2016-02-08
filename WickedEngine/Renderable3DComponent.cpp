#include "Renderable3DComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiLoader.h"
#include "TextureMapping.h"

Renderable3DComponent::Renderable3DComponent()
{
	_needToUpdateRenderData.store(true);

	Renderable2DComponent();
}
Renderable3DComponent::~Renderable3DComponent()
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}
}

void Renderable3DComponent::setProperties()
{
	setLightShaftQuality(0.4f);
	setBloomDownSample(4.0f);
	setAlphaParticleDownSample(1.0f);
	setAdditiveParticleDownSample(1.0f);
	setReflectionQuality(0.5f);
	setSSAOQuality(0.5f);
	setSSAOBlur(2.3f);
	setBloomStrength(19.3f);
	setBloomThreshold(0.99f);
	setBloomSaturation(-3.86f);
	setWaterPlane(wiWaterPlane());
	setDepthOfFieldFocus(10.f);
	setDepthOfFieldStrength(2.2f);

	setSSAOEnabled(true);
	setSSREnabled(true);
	setReflectionsEnabled(false);
	setShadowsEnabled(true);
	setFXAAEnabled(true);
	setBloomEnabled(true);
	setColorGradingEnabled(true);
	setEmitterParticlesEnabled(true);
	setHairParticlesEnabled(true);
	setHairParticlesReflectionEnabled(false);
	setVolumeLightsEnabled(true);
	setLightShaftsEnabled(true);
	setLensFlareEnabled(true);
	setMotionBlurEnabled(true);
	setSSSEnabled(true);
	setDepthOfFieldEnabled(false);

	setPreferredThreadingCount(0);
}

void Renderable3DComponent::Initialize()
{
	Renderable2DComponent::Initialize();

	rtSSR.Initialize(
		(UINT)(wiRenderer::GetScreenWidth()), (UINT)(wiRenderer::GetScreenHeight())
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
	rtMotionBlur.Initialize(
		(UINT)(wiRenderer::GetScreenWidth()), (UINT)(wiRenderer::GetScreenHeight())
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
	rtLinearDepth.Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false, 1, 0, DXGI_FORMAT_R32_FLOAT
		);
	rtParticle.Initialize(
		(UINT)(wiRenderer::GetScreenWidth()*getAlphaParticleDownSample()), (UINT)(wiRenderer::GetScreenHeight()*getAlphaParticleDownSample())
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtParticleAdditive.Initialize(
		(UINT)(wiRenderer::GetScreenWidth()*getAdditiveParticleDownSample()), (UINT)(wiRenderer::GetScreenHeight()*getAdditiveParticleDownSample())
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtWaterRipple.Initialize(
		wiRenderer::GetScreenWidth()
		, wiRenderer::GetScreenHeight()
		, 1, false, 1, 0, DXGI_FORMAT_R8G8B8A8_SNORM
		);
	rtTransparent.Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtVolumeLight.Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false
		);
	rtReflection.Initialize(
		(UINT)(wiRenderer::GetScreenWidth() * getReflectionQuality())
		, (UINT)(wiRenderer::GetScreenHeight() * getReflectionQuality())
		, 1, true, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtFinal[0].Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
	rtFinal[1].Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false);

	rtDof[0].Initialize(
		(UINT)(wiRenderer::GetScreenWidth()*0.5f), (UINT)(wiRenderer::GetScreenHeight()*0.5f)
		, 1, false);
	rtDof[1].Initialize(
		(UINT)(wiRenderer::GetScreenWidth()*0.5f), (UINT)(wiRenderer::GetScreenHeight()*0.5f)
		, 1, false);
	rtDof[2].Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false);

	dtDepthCopy.Initialize(wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, 0
		);

	rtSSAO.resize(3);
	for (unsigned int i = 0; i<rtSSAO.size(); i++)
		rtSSAO[i].Initialize(
			(UINT)(wiRenderer::GetScreenWidth()*getSSAOQuality()), (UINT)(wiRenderer::GetScreenHeight()*getSSAOQuality())
			, 1, false, 1, 0, DXGI_FORMAT_R8_UNORM
			);
	rtSSS.resize(3);
	for (int i = 0; i < 3; ++i)
		rtSSS[i].Initialize(
			(UINT)(wiRenderer::GetScreenWidth()), (UINT)(wiRenderer::GetScreenHeight())
			, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);


	rtSun.resize(2);
	rtSun[0].Initialize(
		wiRenderer::GetScreenWidth()
		, wiRenderer::GetScreenHeight()
		, 1, true
		);
	rtSun[1].Initialize(
		(UINT)(wiRenderer::GetScreenWidth()*getLightShaftQuality())
		, (UINT)(wiRenderer::GetScreenHeight()*getLightShaftQuality())
		, 1, false
		);
	rtLensFlare.Initialize(wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight(), 1, false);

	rtBloom.resize(3);
	rtBloom[0].Initialize(
		wiRenderer::GetScreenWidth()
		, wiRenderer::GetScreenHeight()
		, 1, false, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0
		);
	for (unsigned int i = 1; i<rtBloom.size(); ++i)
		rtBloom[i].Initialize(
		(UINT)(wiRenderer::GetScreenWidth() / getBloomDownSample())
		, (UINT)(wiRenderer::GetScreenHeight() / getBloomDownSample())
		, 1, false
		);
}

void Renderable3DComponent::Load()
{
	Renderable2DComponent::Load();

	wiRenderer::HAIRPARTICLEENABLED = getHairParticlesEnabled();
	wiRenderer::EMITTERSENABLED = getEmittedParticlesEnabled();
}

void Renderable3DComponent::Start()
{
	Renderable2DComponent::Start();
}

void Renderable3DComponent::Update(){
	wiRenderer::Update();
	wiRenderer::SychronizeWithPhysicsEngine();
	wiRenderer::UpdateImages();

	_needToUpdateRenderData.store(true);


	Renderable2DComponent::Update();
}

void Renderable3DComponent::Compose(){
	RenderableComponent::Compose();

	RenderColorGradedComposition();

	Renderable2DComponent::Compose();
}

void Renderable3DComponent::RenderFrameSetUp(DeviceContext context) {
	if (!_needToUpdateRenderData.load())
	{
		return;
	}

	wiRenderer::UpdateRenderData(context);

	_needToUpdateRenderData.store(false);
}
void Renderable3DComponent::RenderReflections(DeviceContext context){
	if (!getReflectionsEnabled() || getReflectionQuality() < 0.01f)
	{
		return;
	}

	rtReflection.Activate(context); {
		// reverse clipping if underwater
		XMFLOAT4 water = getWaterPlane().getXMFLOAT4();
		if (XMVectorGetX(XMPlaneDot(XMLoadFloat4(&getWaterPlane().getXMFLOAT4()), wiRenderer::getCamera()->GetEye())) < 0 )
		{
			water.x *= -1;
			water.y *= -1;
			water.z *= -1;
		}

		wiRenderer::SetClipPlane(water, context);

		wiRenderer::DrawWorld(wiRenderer::getRefCamera(), false, 0, context
			, false, true, SHADERTYPE_NONE
			, nullptr, getHairParticlesReflectionEnabled(), GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::DrawSky(context,true);
	}
}
void Renderable3DComponent::RenderShadows(DeviceContext context){
	if (!getShadowsEnabled())
	{
		return;
	}

	wiRenderer::ClearShadowMaps(context);
	wiRenderer::DrawForShadowMap(context);
}
void Renderable3DComponent::RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, DeviceContext context)
{
	if (getLensFlareEnabled())
	{
		rtLensFlare.Activate(context);
		if (!wiRenderer::GetRasterizer())
			wiRenderer::DrawLensFlares(context);
	}

	if (getVolumeLightsEnabled())
	{
		wiRenderer::UnbindTextures(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, context);
		rtVolumeLight.Activate(context, mainRT.depth);
		wiRenderer::DrawVolumeLights(wiRenderer::getCamera(), context);
	}

	if (getEmittedParticlesEnabled())
	{
		rtParticle.Activate(context, 0, 0, 0, 0);  //OFFSCREEN RENDER ALPHAPARTICLES
		wiRenderer::DrawSoftParticles(wiRenderer::getCamera(), context);
		
		rtParticleAdditive.Activate(context, 0, 0, 0, 1);  //OFFSCREEN RENDER ADDITIVEPARTICLES
		wiRenderer::DrawSoftPremulParticles(wiRenderer::getCamera(), context);
	}

	rtWaterRipple.Activate(context, 0, 0, 0, 0); {
		wiRenderer::DrawWaterRipples(context);
	}

	//wiRenderer::UnbindTextures(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, context);
	rtTransparent.Activate(context, mainRT.depth); {
		wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), shadedSceneRT.shaderResource.front(), rtReflection.shaderResource.front()
			, rtWaterRipple.shaderResource.back(), context);
		wiRenderer::DrawTrails(context, shadedSceneRT.shaderResource.front());
	}

}
void Renderable3DComponent::RenderBloom(DeviceContext context){

	wiImageEffects fx((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight());

	rtBloom[0].Activate(context);
	{
		fx.bloom.separate = true;
		fx.bloom.saturation = getBloomSaturation();
		fx.bloom.threshold = getBloomThreshold();
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		wiImage::Draw(rtFinal[0].shaderResource.front(), fx, context);
	}


	rtBloom[1].Activate(context); //horizontal
	{
		wiRenderer::GenerateMips(rtBloom[0].shaderResource[0], context);
		wiRenderer::GenerateMips(rtBloom[0].shaderResource[0], context);
		fx.mipLevel = 5.32f;
		fx.blur = getBloomStrength();
		fx.blurDir = 0;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[0].shaderResource.back(), fx, context);
	}
	rtBloom[2].Activate(context); //vertical
	{
		fx.blur = getBloomStrength();
		fx.blurDir = 1;
		fx.blendFlag = BLENDMODE_OPAQUE;
		wiImage::Draw(rtBloom[1].shaderResource.back(), fx, context);
	}
}
void Renderable3DComponent::RenderLightShafts(wiRenderTarget& mainRT, DeviceContext context){
	if (!getLightShaftsEnabled())
	{
		return;
	}

	if (XMVectorGetX(XMVector3Dot(wiRenderer::GetSunPosition(), wiRenderer::getCamera()->GetAt())) < 0)
	{
		return;
	}

	wiImageEffects fx((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight());

	//wiRenderer::UnbindTextures(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, context);
	rtSun[0].Activate(context, mainRT.depth); {
		wiRenderer::DrawSun(context);
	}

	rtSun[1].Activate(context); {
		wiImageEffects fxs = fx;
		fxs.blendFlag = BLENDMODE_ADDITIVE;
		XMVECTOR sunPos = XMVector3Project(wiRenderer::GetSunPosition() * 100000, 0, 0, (float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight(), 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
		{
			XMStoreFloat2(&fxs.sunPos, sunPos);
			wiImage::Draw(rtSun[0].shaderResource.back(), fxs, context);
		}
	}
}
void Renderable3DComponent::RenderComposition1(wiRenderTarget& shadedSceneRT, DeviceContext context){
	wiImageEffects fx((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight());
	fx.presentFullScreen = true;

	rtFinal[0].Activate(context);

	fx.blendFlag = BLENDMODE_OPAQUE;
	wiImage::Draw(shadedSceneRT.shaderResource.front(), fx, context);
	fx.blendFlag = BLENDMODE_ALPHA;
	wiImage::Draw(rtTransparent.shaderResource.back(), fx, context);
	if (getEmittedParticlesEnabled()){
		wiImage::Draw(rtParticle.shaderResource.back(), fx, context);
	}

	fx.blendFlag = BLENDMODE_ADDITIVE;
	if (getVolumeLightsEnabled()){
		wiImage::Draw(rtVolumeLight.shaderResource.back(), fx, context);
	}
	if (getEmittedParticlesEnabled()){
		wiImage::Draw(rtParticleAdditive.shaderResource.back(), fx, context);
	}
	if (getLightShaftsEnabled()){
		wiImage::Draw(rtSun.back().shaderResource.back(), fx, context);
	}
	if (getLensFlareEnabled()){
		wiImage::Draw(rtLensFlare.shaderResource.back(), fx, context);
	}
}
void Renderable3DComponent::RenderComposition2(DeviceContext context){
	wiImageEffects fx((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight());
	fx.blendFlag = BLENDMODE_OPAQUE;

	if (getDepthOfFieldEnabled())
	{
		// downsample + blur
		rtDof[0].Activate(context);
		fx.blur = getDepthOfFieldStrength();
		fx.blurDir = 0;
		wiImage::Draw(rtFinal[0].shaderResource.back(), fx, context);

		rtDof[1].Activate(context);
		fx.blurDir = 1;
		wiImage::Draw(rtDof[0].shaderResource.back(), fx, context);
		fx.blur = 0;
		fx.process.clear();

		// depth of field compose pass
		rtDof[2].Activate(context);
		fx.process.setDOF(getDepthOfFieldFocus());
		fx.setMaskMap(rtDof[1].shaderResource.back());
		//fx.setDepthMap(rtLinearDepth.shaderResource.back());
		wiImage::Draw(rtFinal[0].shaderResource.back(), fx, context);
		fx.setMaskMap(nullptr);
		//fx.setDepthMap(nullptr);
		fx.process.clear();
	}

	rtFinal[1].Activate(context);

	fx.process.setFXAA(getFXAAEnabled());
	if (getDepthOfFieldEnabled())
		wiImage::Draw(rtDof[2].shaderResource.back(), fx, context);
	else
		wiImage::Draw(rtFinal[0].shaderResource.back(), fx, context);
	fx.process.clear();

	if (getBloomEnabled())
	{
		fx.blendFlag = BLENDMODE_ADDITIVE;
		fx.presentFullScreen = true;
		wiImage::Draw(rtBloom.back().shaderResource.back(), fx, context);
	}
}
void Renderable3DComponent::RenderColorGradedComposition(){

	wiImageEffects fx((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight());
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_NEAREST;

	if (getColorGradingEnabled())
	{
		fx.quality = QUALITY_BILINEAR;
		if (wiRenderer::GetColorGrading() != nullptr){
			fx.process.setColorGrade(true);
			fx.setMaskMap(wiRenderer::GetColorGrading());
		}
		else
		{
			fx.process.setColorGrade(true);
			fx.setMaskMap(wiTextureHelper::getInstance()->getColorGradeDefault());
		}
	}
	else
	{
		fx.presentFullScreen = true;
	}

	wiImage::Draw(rtFinal[1].shaderResource.back(), fx);
}


void Renderable3DComponent::setPreferredThreadingCount(unsigned short value)
{
	for (auto& wt : workerThreads)
	{
		delete wt;
	}

	workerThreads.clear();
}