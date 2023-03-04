#pragma once
#include "DataManager.h"
#include "vtkImageAlgorithm.h"

#include <vtkImageData.h>
#include <vtkMetaImageReader.h>
#include <vtkPointData.h>

template<typename T>
class RawManager : public RegularGridManager<T>{
public:
    RawManager(const std::string& path){
        this->reader = vtkMetaImageReader::New();

        this->reader->SetFileName(path.c_str());

        this->reader->Update();
    }

    virtual ~RawManager() = default;
    
    glm::uvec3 getSize(){
        int dims[3];
        this->reader->GetOutput()->GetDimensions(dims);
        return glm::uvec3(dims[0], dims[1], dims[2]);
    }

    void readBlock(const glm::uvec3& offset, const glm::uvec3& size, T* blockOut){
        const glm::uvec3 dataSize = this->getSize();
        reader->GetOutput()->GetPointData()->SetScalars(reader->GetOutput()->GetPointData()->GetArray(0));
    
        const T* data = static_cast<T*>(this->reader->GetOutput()->GetScalarPointer());
        for (uint32_t z = 0; z < size.z; ++z)
            for (uint32_t y = 0; y < size.y; ++y){
                memcpy(blockOut + z * size.y * size.x + y * size.x, 
                    data + (offset.z + z) * dataSize.y * dataSize.x + (offset.y + y) * dataSize.x + offset.x,
                    size.x * sizeof(T));
            }
    }

    void release(){
        if (this->reader)this->reader->Delete();
    }
private:
    vtkMetaImageReader* reader = nullptr;
};