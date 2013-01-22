#include "MForest.h"
#include "Engine.h"
#include "MWRenderEvent.h"
#include "MWEnvironment.h"
#include "MWRenderHelper.h"

namespace Myway {

	static const int MForest_VegMagic = 'MFrt';
	static const int MForest_VegVersion = 0;

	IMP_SLN (MForest);

	MForest::MForest()
	{
		INIT_SLN;

		Init();
	}

	MForest::~MForest()
	{
		Shutdown();

		SHUT_SLN;
	}

	void MForest::Init()
	{
		mShaderLib = ShaderLibManager::Instance()->LoadShaderLib("Tree.ShaderLib", "Tree\\Tree.ShaderLib");
		d_assert (mShaderLib);

		mTech_VegMesh = mShaderLib->GetTechnique("GrassMesh");
		mTech_VegBillboard = mShaderLib->GetTechnique("GrassBillboasrd");
		mTech_VegX2 = mShaderLib->GetTechnique("GrassX2");

		d_assert (mTech_VegX2);

		mVegBlockRect = RectF(0, 0, 0, 0);
		mXVegBlockCount = 0;
		mZVegBlockCount = 0;

		mDefaultTree = new MTree("");
		mTrees.PushBack(mDefaultTree.c_ptr());

		mTech_Branch = mShaderLib->GetTechnique("Branch");

		d_assert (mTech_Branch);
	}

	void MForest::Shutdown()
	{
		UnloadVeg();

		mDefaultTree = NULL;

		d_assert (mTreeInstances.Size() == 0);
		d_assert (mTrees.Size() == 0);
	}

	void MForest::Update()
	{
		for (int i = 0; i < mVegetationBlocks.Size(); ++i)
		{
			mVegetationBlocks[i]->_UpdateGeometry();
		}
	}

	void MForest::UnloadVeg()
	{
		RemoveAllVegetationBlock();

		for (int i = 0; i < mVegetations.Size(); ++i)
		{
			delete mVegetations[i];
		}

		mVegetations.Clear();
	}

	void MForest::_AddVisibleVegetationBlock(MVegetationBlock * block)
	{
		mVisibleVegetationBlocks.PushBack(block);
	}

	void MForest::LoadVeg(const TString128 & source)
	{
		UnloadVeg();

		DataStreamPtr stream = ResourceManager::Instance()->OpenResource(source.c_str());

		if (stream == NULL)
			return ;

		int magic, version;

		stream->Read(&magic, sizeof(int));
		stream->Read(&version, sizeof(int));

		d_assert (magic == MForest_VegMagic);
		d_assert (version == MForest_VegVersion);
		
		int vegCount;
		stream->Read(&vegCount, sizeof(int));

		for (int i = 0; i < vegCount; ++i)
		{
			TString32 Name;
			int Type;
			TString128 MeshFile, DiffuseMap, NormalMap, SpecularMap;

			stream->Read(Name.c_str(), 32);
			stream->Read(&Type, sizeof(int));
			stream->Read(MeshFile.c_str(), 128);
			stream->Read(DiffuseMap.c_str(), 128);
			stream->Read(NormalMap.c_str(), 128);
			stream->Read(SpecularMap.c_str(), 128);

			AddVegetation(Name, (MVegetation::GeomType)Type, MeshFile, DiffuseMap, NormalMap, SpecularMap);
		}

		RectF rect;
		int xBlockCount, zBlockCount;

		stream->Read(&rect, sizeof(rect));
		stream->Read(&xBlockCount, sizeof(int));
		stream->Read(&zBlockCount, sizeof(int));

		CreateVegetationBlocks(rect, xBlockCount, zBlockCount);

		for (int j = 0; j < zBlockCount; ++j)
		{
			for (int i = 0; i < xBlockCount; ++i)
			{
				MVegetationBlock * block = GetVegetationBlock(i, j);
				List<MVegetationBlock::Inst> & instList = block->_getInstList();

				int instCount;
				TString32 Name;
				Vec3 Position;
				float Size;

				stream->Read(&instCount, sizeof(int));

				for (int k = 0; k < instCount; ++k)
				{
					stream->Read(Name.c_str(), 32);
					stream->Read(&Position, sizeof(Vec3));
					stream->Read(&Size, sizeof(float));

					MVegetationBlock::Inst inst;

					inst.Veg = GetVegetationByName(Name);
					inst.Position = Position;
					inst.Size = Size;

					instList.PushBack(inst);
				}

				block->_notifyNeedUpdate();
			}
		}
	}

	void MForest::SaveVeg(const TString128 & source)
	{
		File file;

		file.Open(source.c_str(), OM_WRITE_BINARY);

		file.Write(&MForest_VegMagic, sizeof(int));
		file.Write(&MForest_VegVersion, sizeof(int));

		// save vegetation
		int vegCount = mVegetations.Size();
		file.Write(&vegCount, sizeof(int));

		for (int i = 0; i < mVegetations.Size(); ++i)
		{
			MVegetation * veg = mVegetations[i];
			TString32 Name = veg->Name;
			int Type = veg->Type;
			TString128 MeshFile = veg->pMesh != NULL ? veg->pMesh->GetSourceName() : "";
			TString128 DiffuseMap = veg->DiffuseMap != NULL ? veg->DiffuseMap->GetSourceName() : "";
			TString128 NormalMap = veg->NormalMap != NULL ? veg->NormalMap->GetSourceName() : "";
			TString128 SpecularMap = veg->SpecularMap != NULL ? veg->SpecularMap->GetSourceName() : "";

			file.Write(Name.c_str(), 32);
			file.Write(&Type, sizeof(int));
			file.Write(MeshFile.c_str(), 128);
			file.Write(DiffuseMap.c_str(), 128);
			file.Write(NormalMap.c_str(), 128);
			file.Write(SpecularMap.c_str(), 128);
		}

		// save block
		file.Write(&mVegBlockRect, sizeof(RectF));
		file.Write(&mXVegBlockCount, sizeof(int));
		file.Write(&mZVegBlockCount, sizeof(int));

		for (int j = 0; j < mZVegBlockCount; ++j)
		{
			for (int i = 0; i < mXVegBlockCount; ++i)
			{
				MVegetationBlock * block = GetVegetationBlock(i, j);
				int count = block->_getInstanceSize();
				List<MVegetationBlock::Inst> & insts = block->_getInstList();

				file.Write(&count, sizeof(int));

				List<MVegetationBlock::Inst>::Iterator whr = insts.Begin();
				List<MVegetationBlock::Inst>::Iterator end = insts.End();

				while (whr != end)
				{
					const TString32 & Name = whr->Veg->Name;
					const Vec3 & Position = whr->Position;
					float Size = whr->Size;

					file.Write(Name.c_str(), 32);
					file.Write(&Position, sizeof(Vec3));
					file.Write(&Size, sizeof(float));

					++whr;
				}
			}
		}
	}

	void MForest::AddVegetation(const TString32 & name, MVegetation::GeomType type,
								const TString128 & mesh, const TString128 & diffueMap,
								const TString128 & normalMap, const TString128 & specularMap)
	{
		MVegetation * veg = new MVegetation;

		veg->Name = name;
		veg->Type = type;

		if (mesh != "" && type == MVegetation::GT_Mesh)
			veg->pMesh = MeshManager::Instance()->Load(mesh, mesh);

		veg->DiffuseMap = VideoBufferManager::Instance()->Load2DTexture(diffueMap, diffueMap);

		if (normalMap != "" && type == MVegetation::GT_Mesh)
			veg->NormalMap = VideoBufferManager::Instance()->Load2DTexture(normalMap, normalMap);

		if (specularMap != "" && type == MVegetation::GT_Mesh)
			veg->SpecularMap = VideoBufferManager::Instance()->Load2DTexture(specularMap, specularMap);

		mVegetations.PushBack(veg);
	}

	void MForest::RemoveVegetation(MVegetation * veg)
	{
		for (int i = 0; i < mVegetations.Size(); ++i)
		{
			if (mVegetations[i] == veg)
			{
				_OnVegRemoved(veg);

				delete veg;

				mVegetations.Erase(i);

				return ;
			}
		}

		d_assert (0);
	}

	int MForest::GetVegetationCount() const
	{
		return mVegetations.Size();
	}

	MVegetation * MForest::GetVegetation(int index)
	{
		return mVegetations[index];
	}

	MVegetation * MForest::GetVegetationByName(const TString32 & name)
	{
		for (int i = 0; i < mVegetations.Size(); ++i)
		{
			MVegetation * veg = mVegetations[i];

			if (veg->Name == name)
				return veg;
		}

		return NULL;
	}

	void MForest::OnVegetationChanged(MVegetation * veg)
	{
		for (int i = 0; i < mVegetationBlocks.Size(); ++i)
		{
			MVegetationBlock * block = mVegetationBlocks[i];
			block->_OnVegChanged(veg);
		}
	}

	void MForest::AddVegetationInst(MVegetation * veg, const Vec3 & position, float size)
	{
		for (int i = 0; i < mVegetationBlocks.Size(); ++i)
		{
			MVegetationBlock * block = mVegetationBlocks[i];
			const RectF & rc = block->GetRect();

			if (position.x >= rc.x1 && position.x <= rc.x2 &&
				position.z >= rc.y1 && position.z <= rc.y2)
			{
				block->AddVegetation(veg, position, size);
				break;
			}
		}
	}

	void MForest::RemoveVegetationInst(const RectF & rc, MVegetation * veg)
	{
		for (int i = 0; i < mVegetationBlocks.Size(); ++i)
		{
			MVegetationBlock * block = mVegetationBlocks[i];

			if (block->_getInstanceSize() == 0)
				continue;

			const RectF & rcBlock = block->GetRect();

			if (rc.x1 > rcBlock.x2)
				continue;

			if (rc.x2 < rcBlock.x1)
				continue;

			if (rc.y1 > rcBlock.y2)
				continue;

			if (rc.y2 < rcBlock.y1)
				continue;

			block->RemoveVegetation(veg, rc);
		}
	}

	void MForest::CreateVegetationBlocks(const RectF & rect, int xCount, int zCount)
	{
		d_assert (xCount > 0 && zCount > 0);

		mXVegBlockCount = xCount;
		mZVegBlockCount = zCount;
		mVegBlockRect = rect;

		float xStep = (rect.x2 - rect.x1) / xCount;
		float zStep = (rect.y2 - rect.y1) / zCount;

		for (int j = 0; j < zCount; ++j)
		{
			for (int i = 0; i < xCount; ++i)
			{
				RectF rc;

				rc.x1 = rect.x1 + xStep * i;
				rc.y1 = rect.y1 + zStep * j;
				rc.x2 = rc.x1 + xStep;
				rc.y2 = rc.y1 + zStep;

				MVegetationBlock * block = new MVegetationBlock(i, j, rc);

				mVegetationBlocks.PushBack(block);
			}
		}
	}

	MVegetationBlock * MForest::GetVegetationBlock(int x, int z)
	{
		d_assert (x < mXVegBlockCount && z < mZVegBlockCount);

		return mVegetationBlocks[z * mXVegBlockCount + x];
	}

	int MForest::GetVegetationBlockCount()
	{
		return mVegetationBlocks.Size();
	}

	void MForest::RemoveAllVegetationBlock()
	{
		for (int i = 0; i < mVegetationBlocks.Size(); ++i)
		{
			delete mVegetationBlocks[i];
		}

		mVegetationBlocks.Clear();
	}

	void MForest::_preVisibleCull()
	{
		mVisibleVegetationBlocks.Clear();
		mVisbleTreeInstances.Clear();
	}

	void MForest::_render()
	{
		_drawMeshVeg();
		_drawBillboardVeg();
		_drawX2Veg();

		_drawBranch();
	}

	void MForest::_drawMeshVeg()
	{
	}

	void MForest::_drawBillboardVeg()
	{
	}

	void MForest::_drawX2Veg()
	{
		ShaderParam * uNormal = mTech_VegX2->GetVertexShaderParamTable()->GetParam("normal");

		Vec3 normal = -Environment::Instance()->GetEvParam()->LightDir;

		normal = normal.TransformN(World::Instance()->MainCamera()->GetViewMatrix());
		normal.NormalizeL();

		uNormal->SetUnifom(normal.x, normal.y, normal.z, 0);

		for (int i = 0; i < mVisibleVegetationBlocks.Size(); ++i)
		{
			MVegetationBlock * block = mVisibleVegetationBlocks[i];

			for (int j = 0; j < block->GetRenderOpCount(); ++j)
			{
				MVegetation * veg = block->GetRenderVegetation(j);

				if (veg->Type != MVegetation::GT_X2)
					continue;

				RenderOp * rop = block->GetRenderOp(j);

				SamplerState state;

				RenderSystem::Instance()->SetTexture(0, state, veg->DiffuseMap.c_ptr());
				RenderSystem::Instance()->Render(mTech_VegX2, rop);
			}
		}
	}


	void MForest::_OnVegRemoved(MVegetation * veg)
	{
		for (int i = 0; i < mVegetationBlocks.Size(); ++i)
		{
			mVegetationBlocks[i]->_OnVegRemoved(veg);
		}
	}


	MTreePtr MForest::LoadTree(const TString128 & source)
	{
		for (int i = 0; i < mTrees.Size(); ++i)
		{
			if (mTrees[i]->GetSourceName() == source)
				return mTrees[i];
		}

		MTree * tree = new MTree(source);

		tree->Load();

		mTrees.PushBack(tree);

		return tree;
	}

	void MForest::DeleteTree(MTree * tree)
	{
		for (int i = 0; i < mTrees.Size(); ++i)
		{
			if (mTrees[i] == tree)
			{
				mTrees.Erase(i);
				delete tree;
				return ;
			}
		}

		d_assert (0);
	}

	MTreeInstance * MForest::CreateTreeInstance(const TString128 & name, const TString128 & source)
	{
		d_assert (GetTreeInstance(name) == NULL && source != "");

		MTreeInstance * inst = new MTreeInstance(name);

		inst->SetTree(source);

		mTreeInstances.PushBack(inst);

		return inst;
	}

	MTreeInstance * MForest::CreateTreeInstance(const TString128 & name)
	{
		d_assert (GetTreeInstance(name) == NULL);

		MTreeInstance * inst = new MTreeInstance(name);

		inst->SetTree(mDefaultTree);

		mTreeInstances.PushBack(inst);

		return inst;
	}

	MTreeInstance * MForest::GetTreeInstance(const TString128 & name)
	{
		for (int i = 0; i < mTreeInstances.Size(); ++i)
		{
			if (mTreeInstances[i]->GetName() == name)
				return mTreeInstances[i];
		}

		return NULL;
	}

	bool MForest::RenameTreeInstance(const TString128 & name, MTreeInstance * inst)
	{
		if (inst->GetName() != name && GetTreeInstance(name) == NULL)
		{
			inst->SetName(name);
			return true;
		}

		return false;
	}

	void MForest::DestroyInstance(MTreeInstance * tree)
	{
		for (int i = 0; i < mTreeInstances.Size(); ++i)
		{
			if (mTreeInstances[i] == tree)
			{
				delete tree;
				mTreeInstances.Erase(i);
				return ;
			}
		}

		d_assert (0);
	}

	void MForest::_AddVisibleTreeInstance(MTreeInstance * tree)
	{
		mVisbleTreeInstances.PushBack(tree);
	}

	void MForest::_drawBranch()
	{
		RenderSystem::Instance()->_BeginEvent("Draw Branch");

		ShaderParam * uUVScale = mTech_Branch->GetVertexShaderParamTable()->GetParam("gUVScale");

		for (int i = 0; i < mVisbleTreeInstances.Size(); ++i)
		{
			MTreeInstance * inst = mVisbleTreeInstances[i];
			MTree * tree = inst->GetTree().c_ptr();
			RenderOp * rop = tree->_getBranchRenderOp();

			const Vec2 & uvScale = tree->GetTreeResource()->GetBranch()->GetDiffuseUVScale();

			uUVScale->SetUnifom(uvScale.x, uvScale.y, 0, 0);

			SamplerState state;

			rop->xform = inst->GetAttachNode()->GetWorldMatrix();

			TexturePtr diffuseTex = tree->GetTreeResource()->GetBranchTexture(MTreeRes::TT_Diffuse);

			RenderSystem::Instance()->SetTexture(0, state, diffuseTex.c_ptr());
			RenderSystem::Instance()->Render(mTech_Branch, rop);
		}

		RenderSystem::Instance()->_EndEvent();
	}












	MForestListener gForestListener;

	MForestListener::MForestListener()
		: OnInit(&RenderEvent::OnEngineInit, this, &MForestListener::_init)
		, OnShutdown(&RenderEvent::OnEngineShutdown, this, &MForestListener::_shutdown)
		, OnUpdate(&RenderEvent::OnPostUpdateScene, this, &MForestListener::_update)
		, OnRender(&RenderEvent::OnAfterRenderSolid, this, &MForestListener::_render)
		, OnPreVisibleCull(&RenderEvent::OnPreVisibleCull, this, &MForestListener::_preVisibleCull)
	{
	}

	MForestListener::~MForestListener()
	{
	}

	void MForestListener::_init(void * param0, void * param1)
	{
		mForest = new MForest();
	}

	void MForestListener::_shutdown(void * param0, void * param1)
	{
		delete mForest;
	}

	void MForestListener::_update(void * param0, void * param1)
	{
		mForest->Update();
	}

	void MForestListener::_render(void * param0, void * param1)
	{
		mForest->_render();
	}

	void MForestListener::_preVisibleCull(void * param0, void * param1)
	{
		mForest->_preVisibleCull();
	}
}

