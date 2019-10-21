#include "objmesh.h"
#include "shapes/triangle.h"
#include "textures/constant.h"
#include "paramset.h"
#include "ext/rply.h"

#include <iostream>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace pbrt {
using namespace std;


std::vector<std::shared_ptr<Shape>> pbrt::CreateObjMesh(
    const Transform* o2w, const Transform* w2o, bool reverseOrientation,
    const ParamSet& params,
    std::map<std::string, std::shared_ptr<Texture<Float>>>* floatTextures) {

	const std::string filename = params.FindOneFilename("filename", "");

	std::string err;
    std::vector<tinyobj::shape_t> m_shapes;
    std::vector<tinyobj::material_t> tinyobj_materials;
    bool ret = tinyobj::LoadObj(m_shapes, tinyobj_materials, err, filename.c_str(),"");

    if (!err.empty()) std::cerr << err << std::endl;
    if (!ret) {
       std::cerr << "Failed to load OBJ scene '" << filename
                      << "': " << err << std::endl;
       throw std::runtime_error(err.c_str());
    }

	//
	// scan mesh for normals and texcoords
	// 
	uint64_t num_groups_with_normals = 0;
	uint64_t num_groups_with_texcoords = 0;
	bool has_normals = false;
	bool has_texcoords = false;

	for (std::vector<tinyobj::shape_t>::const_iterator it = m_shapes.begin();
		it < m_shapes.end();
		++it) {
		const tinyobj::shape_t& shape = *it;

		if (!shape.mesh.normals.empty())
			++num_groups_with_normals;

		if (!shape.mesh.texcoords.empty())
			++num_groups_with_texcoords;
	}

	if (num_groups_with_normals != 0)
	{
		if (num_groups_with_normals != m_shapes.size())
			std::cerr << "OBJ Loader - WARNING: mesh '" << filename
			<< "' has normals for some groups but not all.  "
			<< "Ignoring all normals." << std::endl;
		else
			has_normals = true;

	}
	if (num_groups_with_texcoords != 0)
	{
		if (num_groups_with_texcoords != m_shapes.size())
			std::cerr << "OBJ Loader - WARNING: mesh '" << filename
			<< "' has texcoords for some groups but not all.  "
			<< "Ignoring all texcoords." << std::endl;
		else
			has_texcoords = true;
	}


	//
	// shapes
	// 

	std::vector<int> I;
	std::vector<Point3f> P;
	std::vector<Normal3f> N;
	std::vector<Point2f> UV;
	
	for (std::vector<tinyobj::shape_t>::const_iterator it = m_shapes.begin();
		it < m_shapes.end();
		++it) {

		const tinyobj::shape_t& shape = *it;

		std::cerr << "Load shape: " << shape.name << std::endl
			<< (shape.mesh.positions.size() / 3) << ", "
			<< (shape.mesh.indices.size() / 3) << ", "
			<< (shape.mesh.normals.size() / 3) << ", "
			<< (shape.mesh.texcoords.size() / 2) << std::endl;

		
			
		for (uint64_t i = 0; i < shape.mesh.positions.size() / 3; ++i)
		{
			const float x = shape.mesh.positions[i * 3 + 0];
			const float y = shape.mesh.positions[i * 3 + 1];
			const float z = shape.mesh.positions[i * 3 + 2];
			P.push_back(Point3f(x, y, z));
			
		}
		

		for (uint64_t i = 0; i < shape.mesh.indices.size(); ++i)
		{
			I.push_back(uint32_t(shape.mesh.indices[i]));
		}

		
		if (has_normals) {
			for (uint64_t i = 0; i < shape.mesh.normals.size() / 3; ++i) {
				const float x = shape.mesh.normals[i * 3 + 0];
				const float y = shape.mesh.normals[i * 3 + 1];
				const float z = shape.mesh.normals[i * 3 + 2];
				N.push_back(Normal3f(x, y, z));
			}
		}

		if (has_texcoords) {
			for (uint64_t i = 0; i < shape.mesh.texcoords.size() / 2; ++i) {
				const float x = shape.mesh.texcoords[i * 2 + 0];
				const float y = shape.mesh.texcoords[i * 2 + 1];

				UV.push_back(Point2f(x, y));
			}
		}
		


	}
   

	return CreateTriangleMesh(o2w, w2o, reverseOrientation,
		I.size() / 3, I.data(),
		P.size(), P.data(), nullptr, N.data(),
		UV.data(), nullptr, nullptr,
		nullptr);
}

}  // namespace pbrt