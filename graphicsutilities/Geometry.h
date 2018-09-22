/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/


#ifndef GEOMETRY_H_INCLUDED
#define GEOMETRY_H_INCLUDED

#include "..\buildsettings.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\intersection.h"
#include "..\graphicsutilities\AnimationUtilities.h"


namespace FlexKit
{
	enum class VERTEXBUFFER_FORMAT
	{
		VERTEXBUFFER_FORMAT_UNKNOWN			= -1,
		VERTEXBUFFER_FORMAT_R8				= 1,
		VERTEXBUFFER_FORMAT_R8G8B8			= 3,
		VERTEXBUFFER_FORMAT_R8G8B8A8		= 8,
		VERTEXBUFFER_FORMAT_R16				= 2,
		VERTEXBUFFER_FORMAT_R16G16			= 4,
		VERTEXBUFFER_FORMAT_R16G16B16		= 6,
		VERTEXBUFFER_FORMAT_R16G16B16A16	= 8,
		VERTEXBUFFER_FORMAT_R32				= 4,
		VERTEXBUFFER_FORMAT_R32G32			= 8,
		VERTEXBUFFER_FORMAT_R32G32B32		= 12,
		VERTEXBUFFER_FORMAT_R32G32B32A32	= 16,
		VERTEXBUFFER_FORMAT_MATRIX			= 64,
		VERTEXBUFFER_FORMAT_COMBINED		= 32
	};


	/************************************************************************************************/


	enum class VERTEXBUFFER_TYPE
	{
		VERTEXBUFFER_TYPE_COLOR,
		VERTEXBUFFER_TYPE_NORMAL,
		VERTEXBUFFER_TYPE_TANGENT,
		VERTEXBUFFER_TYPE_UV,
		VERTEXBUFFER_TYPE_POSITION,
		VERTEXBUFFER_TYPE_USERTYPE,
		VERTEXBUFFER_TYPE_USERTYPE2,
		VERTEXBUFFER_TYPE_USERTYPE3,
		VERTEXBUFFER_TYPE_USERTYPE4,
		VERTEXBUFFER_TYPE_COMBINED,
		VERTEXBUFFER_TYPE_PACKED,
		VERTEXBUFFER_TYPE_PACKEDANIMATION,
		VERTEXBUFFER_TYPE_INDEX,
		VERTEXBUFFER_TYPE_ANIMATION1,
		VERTEXBUFFER_TYPE_ANIMATION2,
		VERTEXBUFFER_TYPE_ANIMATION3,
		VERTEXBUFFER_TYPE_ANIMATION4,
		VERTEXBUFFER_TYPE_ERROR
	};


	/************************************************************************************************/


	class FLEXKITAPI VertexBufferView
	{
	public:
		VertexBufferView();
		VertexBufferView(FlexKit::byte* _ptr, size_t size);
		~VertexBufferView();

		VertexBufferView  operator + (const VertexBufferView& RHS);

		VertexBufferView& operator = (const VertexBufferView& RHS);	// Assignment Operator
		//VertexBuffer& operator = ( const VertexBuffer&& RHS );	// Movement Operator


		template<typename Ty>
		class Typed_Iteration
		{
			typedef Typed_Iteration<Ty> This_Type;
		public:
			Typed_Iteration(FlexKit::byte* _ptr, size_t Size) : m_position((Ty*)(_ptr)), m_size(Size) {}

			class Typed_iterator
			{
			public:
				Typed_iterator(Ty* _ptr) : m_position(_ptr) {}
				Typed_iterator(const Typed_iterator& rhs) : m_position(rhs.m_position) {}

				inline void operator ++ ()		{ m_position++; }
				inline void operator ++ (int)	{ m_position++; }
				inline void operator -- ()		{ m_position--; }
				inline void operator -- (int)	{ m_position--; }

				inline Typed_iterator	operator =	(const Typed_iterator& rhs) {}

				inline bool operator <	(const Typed_iterator& rhs) { return (this->m_position < rhs.m_position); }


				inline bool				operator == (const Typed_iterator& rhs) { return  (m_position == &*rhs); }
				inline bool				operator != (const Typed_iterator& rhs) { return !(m_position == &*rhs); }

				inline Ty&				operator * () { return *m_position; }
				inline Ty				operator * ()	const { return *m_position; }

				inline const Ty&		peek_forward()	const { return *(m_position + sizeof(Ty)); }

			private:

				Ty*	m_position;
			};

			Ty&	operator [] (size_t index) { return m_position[index]; }
			Typed_iterator begin() { return Typed_iterator(m_position); }
			Typed_iterator end() { return Typed_iterator(m_position + m_size); }

			inline const size_t	size() { return m_size / sizeof(Ty); }

		private:
			Ty*			m_position;
			size_t		m_size;
		};

				/************************************************************************************************/


		template< typename TY >
		inline bool Push(TY& in)
		{
			if (mBufferUsed + sizeof(TY) > mBufferSize)
				return false;

			auto size = sizeof(TY);
			if (sizeof(TY) != static_cast<uint32_t>(mBufferFormat))
				mBufferinError = true;
			else if (!mBufferinError)
			{
				char* Val = (char*)&in;
				for (size_t itr = 0; itr < static_cast<uint32_t>(mBufferFormat); itr++)
					mBuffer[mBufferUsed++] = Val[itr];
			}
			return !mBufferinError;
		}


		/************************************************************************************************/


		template< typename TY >
		inline bool Push(TY& in, size_t bytesize)
		{
			if (mBufferUsed + bytesize > mBufferSize)
				return false;

			char* Val = (char*)&in;
			for (size_t itr = 0; itr < bytesize; itr++)
				mBuffer.push_back(Val[itr]);

			return !mBufferinError;
		}


		/************************************************************************************************/


		template<>
		inline bool Push(float3& in)
		{
			if (mBufferUsed + static_cast<uint32_t>(mBufferFormat) > mBufferSize)
				return false;

			char* Val = (char*)&in;
			for (size_t itr = 0; itr < static_cast<uint32_t>(mBufferFormat); itr++)
				mBuffer[mBufferUsed++] = Val[itr];
			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename TY>
		inline bool Push(TY* in)
		{
			if (mBufferUsed + sizeof(TY) > mBufferSize)
				return false;

			if (!mBufferinError)
			{
				char Val[128];
				memcpy(Val, &in, mBufferFormat);

				for (size_t itr = 0; itr < static_cast<uint32_t>(mBufferFormat); itr++)
					mBuffer.push_back(Val[itr]);
			}
			return !mBufferinError;
		}


		/************************************************************************************************/


		template<typename Ty>
		inline Typed_Iteration<Ty> CreateTypedProxy()
		{
			return Typed_Iteration<Ty>(GetBuffer(), GetBufferSizeUsed());
		}


		template<typename Ty>
		inline bool Push(vector_t<Ty> static_vector)
		{
			if (mBufferUsed + sizeof(Ty)*static_vector.size() > mBufferSize)
				return false;

			for (auto in : static_vector)
				if (!Push(in)) return false;

			return true;
		}


		template<typename Ty, size_t count>
		inline bool Push(static_vector<Ty, count>& static_vector)
		{
			if (mBufferUsed + sizeof(Ty)*static_vector.size() > mBufferSize)
				return false;

			for (auto in : static_vector)
				if (!Push(in)) return false;

			return true;
		}


		void Begin(VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT);
		bool End();

		bool				LoadBuffer();
		bool				UnloadBuffer();

		byte*				GetBuffer()			const;
		size_t				GetElementSize()	const;
		size_t				GetBufferSize()		const;
		size_t				GetBufferSizeUsed()	const;
		size_t				GetBufferSizeRaw()	const;
		VERTEXBUFFER_FORMAT	GetBufferFormat()	const;
		VERTEXBUFFER_TYPE	GetBufferType()		const;

		void SetTypeFormatSize(VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT, size_t count);

	private:
		bool _combine(const VertexBufferView& LHS, const VertexBufferView& RHS, char* out);

		void				SetElementSize(size_t) {}
		byte*				mBuffer;
		size_t				mBufferSize;
		size_t				mBufferUsed;
		size_t				mBufferElementSize;
		VERTEXBUFFER_FORMAT	mBufferFormat;
		VERTEXBUFFER_TYPE	mBufferType;
		bool				mBufferinError;
		bool				mBufferLock;
	};


	/************************************************************************************************/


	FLEXKITAPI VertexBufferView* CreateVertexBufferView(byte* Memory, size_t BufferLength);


	/************************************************************************************************/


	inline void CreateBufferView(size_t size, iAllocator* memory, VertexBufferView*& View, VERTEXBUFFER_TYPE T, VERTEXBUFFER_FORMAT F)
	{
		size_t VertexBufferSize = size * (size_t)F + sizeof(VertexBufferView);
		View = FlexKit::CreateVertexBufferView((byte*)memory->_aligned_malloc(VertexBufferSize), VertexBufferSize);
		View->Begin(T, F);
	}


	/************************************************************************************************/


	template<typename Ty_Container, typename FetchFN, typename TranslateFN>
	bool FillBufferView(Ty_Container* Container, size_t vertexCount, VertexBufferView* out, TranslateFN Translate, FetchFN Fetch)
	{
		for (size_t itr = 0; itr < vertexCount; ++itr) {
			auto temp = Translate(Fetch(itr, Container));
			out->Push(temp);
		}

		return out->End();
	}


	/************************************************************************************************/


	template<typename Ty_Container, typename FetchFN, typename TranslateFN, typename ProcessFN>
	void ProcessBufferView(Ty_Container* Container, size_t vertexCount, TranslateFN Translate, FetchFN Fetch, ProcessFN Process)
	{
		for (size_t itr = 0; itr < vertexCount; ++itr) {
			auto Vert = Translate(Fetch(itr, Container));
			Process(Vert);
		}
	}

	/************************************************************************************************/


	struct SkinDeformer
	{
		const char*	BoneNames;
		size_t		BoneCount;
	};

	union BoundingVolume
	{
		BoundingVolume() {}

		AABB			OBB;
		BoundingSphere	BS;
	};


	struct Vertex
	{
		void Clear() { xyz = { 0.0f, 0.0f, 0.0f }; }

		void AddWithWeight(Vertex const &src, float weight)
		{
			xyz.x += weight * src.xyz.x;
			xyz.y += weight * src.xyz.y;
			xyz.z += weight * src.xyz.z;
		}

		float3 xyz;
	};


	/************************************************************************************************/

	
	enum EAnimationData
	{
		EAD_None	= 0,
		EAD_Skin	= 1,
		EAD_Vertex	= 2
	};

	enum EOcclusion_Volume
	{
		EOV_ORIENTEDBOUNDINGBOX,
		EOV_ORIENTEDSPHERE,
	};


	/************************************************************************************************/


	struct TriMeshResource
	{
		size_t AnimationData;
		size_t IndexCount;
		size_t TriMeshID;
		size_t IndexBuffer_Idx;

		struct SubDivInfo
		{
			size_t  numVertices;
			size_t  numFaces;
			int*	numVertsPerFace;
			int*	IndicesPerFace;
		}*SubDiv;

		const char*			ID;
		SkinDeformer*		SkinTable;
		Skeleton*			Skeleton;

		struct RInfo
		{
			float3 Offset;
			float3 min, max;
			float  r;
		}Info;

		// Visibility Information
		AABB			AABB;
		BoundingSphere	BS;

		size_t	SkeletonGUID;

		VertexBufferView*	Buffers[16];
	};

}	/************************************************************************************************/

#endif