// #include "ObjVertInfoFile.hpp"

// // void star::ObjVertTransfer::writeData(star::StarBuffer& buffer) const{
// //     tinyobj::ObjReaderConfig readerConfig;
// //     tinyobj::ObjReader reader;

// //     if (!reader.ParseFromFile(this->filePath, readerConfig)){
// //         if (!reader.Error().empty()){
// //             std::cout << "Reader error: " << reader.Error();
// //         }
// //         exit(1);
// //     }

// //     auto& attrib = reader.GetAttrib();
// //     auto& shapes = reader.GetShapes();

// //     assert(shapeIndex < shapes.size() && "Shape index being used is outside of file");
// // }


// void star::ObjVertTransfer::writeData(star::StarBuffer& buffer) const{
//     buffer.map();

//     auto data = this->getVertices();

//     for (int i = 0; i < data.size(); ++i){
//         buffer.writeToIndex(&data.at(i), i);
//     }

//     buffer.unmap();
// }

// std::vector<star::Vertex> star::ObjVertTransfer::getVertices() const{
//     return vertices;
// }

// std::unique_ptr<star::BufferMemoryTransferRequest> star::ObjVertInfo::createTransferRequest() const{
//     // size_t passOffVerts = 0;
//     // if (this->numVerts.has_value()){
//     //     passOffVerts = this->numVerts.value();
//     // }else{
//     //     passOffVerts = getNumVerts(this->filePath, this->shapeIndex);
//     // }

//     return std::make_unique<ObjVertTransfer>(this->vertices);
// }

// // size_t star::ObjVertInfo::getNumVerts(const std::string& filePath, const size_t& shapeIndex){
// //     tinyobj::ObjReaderConfig readerConfig;
// //     tinyobj::ObjReader reader;

// //     if (!reader.ParseFromFile(filePath, readerConfig)){
// //         if (!reader.Error().empty()){
// //             std::cout << "Reader error: " << reader.Error();
// //         }
// //         exit(1);
// //     }

// //     auto& attrib = reader.GetAttrib();
// //     auto& shapes = reader.GetShapes();

// //     assert(shapeIndex < shapes.size() && "Shape index being used is outside of file");

// //     return attrib.vertices.size();
// // }