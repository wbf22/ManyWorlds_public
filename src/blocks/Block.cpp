// Fill out your copyright notice in the Description page of Project Settings.


#include "Block.h"

Block::Block(string material, shared_ptr<Position> position, shared_ptr<PositionDouble> position_double, double size)
{
    this->material = material;
    this->position = position;
    this->position_double = position_double;
    this->size = size;
    this->render_size = 0.0;
}

Block::Block()
{
}


int Block::getSize() {
    return this->size;
}

int64_t Block::getId() {
    string str = std::to_string(this->getSize()).substr(0, 3) + "0" + std::to_string(this->position->x) + "0" + std::to_string(this->position->z);
    str = str.substr(0, 18);
    return std::stoll(str);
}



string Block::stringTag() {
    return Block::stringTag(this->getSize(), this->position->x, this->position->y, this->position->z);
}


string Block::stringTag(int size, int64_t x, int64_t y, int64_t z) {
    // return std::to_string(size) + " "
    //     + std::to_string(x) + " "
    //     + std::to_string(y) + " "
    //     + std::to_string(z);
    char buf[96]; // enough for four 64-bit ints + spaces
    int len = snprintf(buf, sizeof(buf), "%d %" PRId64 " %" PRId64 " %" PRId64, size, x, y, z);
    return string(buf, len);
}


string Block::stringTag_double(double size, double x, double y, double z) {
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2);
	oss << size << " " << x << " " << y << " " << z;
	return oss.str();
}


void Block::parseStringTagDouble(const string &tag, double &size, double &x, double &y, double &z) {
    string tag_copy = tag;

    int first = tag_copy.find_first_of(" ");
    size = std::stod(tag_copy.substr(0, first));
    
    tag_copy = tag_copy.substr(first + 1);
    first = tag_copy.find_first_of(" ");
    x = std::stod(tag_copy.substr(0, first));

    tag_copy = tag_copy.substr(first + 1);
    first = tag_copy.find_first_of(" ");
    y = std::stod(tag_copy.substr(0, first));

    z = std::stod(tag_copy.substr(first + 1));
}

void Block::parseStringTag(const string &tag, int64_t &size, int64_t &x, int64_t &y, int64_t &z) {
    string tag_copy = tag;

    int first = tag_copy.find_first_of(" ");
    size = std::stoi(tag_copy.substr(0, first));
    
    tag_copy = tag_copy.substr(first + 1);
    first = tag_copy.find_first_of(" ");
    x = std::stoi(tag_copy.substr(0, first));

    tag_copy = tag_copy.substr(first + 1);
    first = tag_copy.find_first_of(" ");
    y = std::stoi(tag_copy.substr(0, first));

    z = std::stoi(tag_copy.substr(first + 1));
}

string Block::stringTagNoY()
{
    return Block::stringTagNoY(this->getSize(), this->position->x, this->position->z);
}

string Block::stringTagNoY(int size, int64_t x, int64_t z)
{
    // return std::to_string(size) + " "
    //     + std::to_string(x) + " "
    //     + std::to_string(z);
    char buf[72]; // enough for three 64-bit ints + spaces
    int len = snprintf(buf, sizeof(buf), "%d %" PRId64 " %" PRId64, size, x, z);
    return string(buf, len);
}

void Block::parseStringTagNoY(const string &tag, int &size, int64_t &x, int64_t &z)
{
    string tag_copy = tag;

    int first = tag_copy.find_first_of(" ");
    size = std::stoi(tag_copy.substr(0, first));
    
    tag_copy = tag_copy.substr(first + 1);
    first = tag_copy.find_first_of(" ");
    x = std::stoi(tag_copy.substr(0, first));

    z = std::stoi(tag_copy.substr(first + 1));
}

shared_ptr<Block> Block::duplicate()
{
    shared_ptr<Block> block = std::make_shared<Block>();
    block->material = this->material;
    block->size = this->size;
    block->render_size = this->render_size;
    if (this->position != nullptr) block->position = std::make_shared<Position>(this->position->x, this->position->y, this->position->z);
    if (this->position_double != nullptr) block->position_double = std::make_shared<PositionDouble>(this->position_double->x, this->position_double->y, this->position_double->z);
    return block;
}
