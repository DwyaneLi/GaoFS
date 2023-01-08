//
// Created by DELL on 2023/1/2.
//

#include <daemon/backend/metadata/merge.hpp>
#include <stdexcept>

using namespace std;

namespace gaofs::metadata {

string MergeOperand::serialize_id() const {
    // return id:
    string s;
    s.reserve(2);
    s += (char)id();
    s += operand_id_suffix;
    return s;
}

string MergeOperand::seriaize() const {
    // return id:params
    string s = serialize_id();
    s += serialize_params();
    return s;
}

OperandID MergeOperand::get_id(const rdb::Slice& serialized_op) {
    return static_cast<OperandID>(serialized_op[0]);
}

rdb::Slice MergeOperand::get_params(const rdb::Slice &serialize_op) {
    assert(serialize_op[1] == operand_id_suffix);
    return {serialize_op.data()+2, serialize_op.size() - 2};
}

IncreaseSizeOperand::IncreaseSizeOperand(size_t size, bool append) : size(size), append(append) {}

IncreaseSizeOperand::IncreaseSizeOperand(const rdb::Slice &serialized_op) {
    size_t chrs_parsed = 0;
    size_t read = 0;

    // 初始化size
    size = stoul(serialized_op.data() + chrs_parsed, &read);
    chrs_parsed += (read + 1);
    assert(serialized_op[chrs_parsed  - 1] == separator);

    // 初始化append
    assert(serialized_op[chrs_parsed] == true_char || serialized_op[chrs_parsed] == false_char);
    append = (serialized_op[chrs_parsed] != false_char);

    assert(chrs_parsed + 1 == serialized_op.size());
}

OperandID IncreaseSizeOperand::id() const {
    return OperandID::increase_size;
}

std::string IncreaseSizeOperand::serialize_params() const {
    string s;
    s.reserve(3);
    s += to_string(size);
    s += this->separator;
    s += (append ? true_char : false_char);
    return s;
}

DecreaseSizeOperand::DecreaseSizeOperand(size_t size) : size(size) {}

DecreaseSizeOperand::DecreaseSizeOperand(const rdb::Slice &serialized_op) {
    size_t read;
    size = stoul(serialized_op.ToString(), &read);
    assert(read == serialized_op.size());
}

OperandID DecreaseSizeOperand::id() const {
    return OperandID::decrease_size;
}

std::string DecreaseSizeOperand::serialize_params() const {
    return to_string(size);
}

CreateOperand::CreateOperand(const std::string &metadata) : metadata(metadata) {}

OperandID CreateOperand::id() const {
    return OperandID::create;
}

std::string CreateOperand::serialize_params() const {
    return metadata;
}

bool MetadataMergeOperator::FullMergeV2(const MergeOperationInput &merge_in, MergeOperationOutput *merge_out) const {
    string prev_md_valiue;
    // operland_list里记录的是写操作序列
    // merge_in里记录了key old_value和写操作序列
    // cbegin只能读，begin可以写
    auto ops_it = merge_in.operand_list.cbegin();
    // 一开始不存在value
    if(merge_in.existing_value == nullptr) {
        // 第一个操作绝对要是创建
        if(MergeOperand::get_id(ops_it[0]) != OperandID::create) {
            throw runtime_error("Merge operation failed: key do not exists and first operand is not a creation");
            // 还可以用logger记录
        }
        prev_md_valiue = MergeOperand::get_params(ops_it[0]).ToString();
        ops_it++;
    } else {
        prev_md_valiue = merge_in.existing_value->ToString();
    }

    // 确定操作的元数据value
    Metadata md{prev_md_valiue};

    size_t fsize = md.size();
    // 按写操作序列进行操作
    for( ; ops_it != merge_in.operand_list.cend(); ops_it++) {
        const rdb::Slice& serialized_op = *ops_it;
        assert(serialized_op.size() >= 2);
        auto operand_id = MergeOperand::get_id(serialized_op);
        auto parameters = MergeOperand::get_params(serialized_op);

        if(operand_id == OperandID::create) {
            continue;
        } else if(operand_id == OperandID::increase_size) {
            auto op = IncreaseSizeOperand(parameters);
            if(op.append) {
                fsize += op.size;
            } else {
                fsize = max(fsize, op.size);
            }
        } else if(operand_id == OperandID::decrease_size) {
            auto op = DecreaseSizeOperand(parameters);
            fsize = op.size;
        } else {
            throw runtime_error("Unrecognized merge operand ID: " + (char)operand_id);
        }
    }

    md.size(fsize);
    merge_out->new_value = md.serialize();
    return true;
}

bool MetadataMergeOperator::PartialMergeMulti(const rdb::Slice &key, const std::deque<rdb::Slice> &operand_list,
                                              std::string *new_value, rdb::Logger *logger) const {
    return false;
}

const char * MetadataMergeOperator::Name() const {
    return "MetadataMergeOperator";
}

bool MetadataMergeOperator::AllowSingleOperand() const {
    return true;
}

} // namespace gaofs::metadata


