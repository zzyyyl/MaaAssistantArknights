#include "TaskData.h"

#include <algorithm>

#include <meojson/json.hpp>

#include "AsstTypes.h"
#include "GeneralConfiger.h"
#include "TemplResource.h"
#include "Logger.hpp"

const std::shared_ptr<asst::TaskInfo> asst::TaskData::get(const std::string& name) noexcept
{
    if (auto iter = m_all_tasks_info.find(name);
        iter != m_all_tasks_info.cend()) {
        return iter->second;
    }
    else {
        int name_split = static_cast<int>(name.find('@') + 1);
        if (name_split == 0) {
            return nullptr;
        }
        if (auto alias_iter = m_all_tasks_info.find(std::string(name, name_split));
            alias_iter != m_all_tasks_info.cend()) {
            if (auto task_info_ptr = generate_task_info(alias_iter->second, std::string(name, 0, name_split));
                task_info_ptr != nullptr) {
                m_all_tasks_info.emplace(name, task_info_ptr);
                return task_info_ptr;
            }
        }
        return nullptr;
    }
}

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept
{
    return m_templ_required;
}

// std::shared_ptr<asst::TaskInfo> asst::TaskData::get(const std::string& name)
// {
//     return m_all_tasks_info[name];
// }

std::shared_ptr<asst::TaskInfo> asst::TaskData::generate_task_info(
    const std::shared_ptr<asst::TaskInfo>& base_ptr, const std::string& task_prefix) const
{
    std::shared_ptr<asst::TaskInfo> task_info_ptr = nullptr;
    switch (base_ptr->algorithm) {
    case AlgorithmType::JustReturn: {   
        auto base_ret_ptr = std::dynamic_pointer_cast<TaskInfo>(base_ptr);
        task_info_ptr = std::make_shared<TaskInfo>(*base_ret_ptr);
    } break;
    case AlgorithmType::MatchTemplate: {
        auto base_match_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(base_ptr);
        task_info_ptr = std::make_shared<MatchTaskInfo>(*base_match_ptr);
    } break;
    case AlgorithmType::OcrDetect: {
        auto base_ocr_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(base_ptr);
        task_info_ptr = std::make_shared<OcrTaskInfo>(*base_ocr_ptr);
    } break;
    case AlgorithmType::Hash: {
        auto base_hash_ptr = std::dynamic_pointer_cast<HashTaskInfo>(base_ptr);
        task_info_ptr = std::make_shared<HashTaskInfo>(*base_hash_ptr);
    } break;
    default:
        return nullptr;
    }
    task_info_ptr->exceeded_next = {};
    for (const std::string& excceed_next : base_ptr->exceeded_next) {
        task_info_ptr->exceeded_next.emplace_back(task_prefix + excceed_next);
    }
    task_info_ptr->next = {};
    for (const std::string& next : base_ptr->next) {
        task_info_ptr->next.emplace_back(task_prefix + next);
    }
    return task_info_ptr;
}

bool asst::TaskData::generate_task_algorithm(std::shared_ptr<asst::TaskInfo>& task_info_ptr, std::string algorithm_str)
{
    static auto to_lower = [](char c) -> char {
        return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    };
    std::transform(algorithm_str.begin(), algorithm_str.end(), algorithm_str.begin(), to_lower);
    if (algorithm_str == "matchtemplate") {
        task_info_ptr = std::make_shared<MatchTaskInfo>();
        task_info_ptr->algorithm = AlgorithmType::MatchTemplate;
    }
    else if (algorithm_str == "justreturn") {
        task_info_ptr = std::make_shared<TaskInfo>();
        task_info_ptr->algorithm = AlgorithmType::JustReturn;
    }
    else if (algorithm_str == "ocrdetect") {
        task_info_ptr = std::make_shared<OcrTaskInfo>();
        task_info_ptr->algorithm = AlgorithmType::OcrDetect;
    }
    else if (algorithm_str == "hash") {
        task_info_ptr = std::make_shared<HashTaskInfo>();
        task_info_ptr->algorithm = AlgorithmType::Hash;
    }
    else {
        m_last_error = "algorithm error " + algorithm_str;
        return false;
    }
    return true;
}

bool asst::TaskData::generate_task_action(
    const std::shared_ptr<asst::TaskInfo>& task_info_ptr,
    const json::value& task_json,
    std::string action_str,
    const asst::Rect& specific_rect_default)
{
    static auto to_lower = [](char c) -> char {
        return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    };
    std::transform(action_str.begin(), action_str.end(), action_str.begin(), to_lower);
    if (action_str == "clickself") {
        task_info_ptr->action = ProcessTaskAction::ClickSelf;
    }
    else if (action_str == "clickrand") {
        task_info_ptr->action = ProcessTaskAction::ClickRand;
    }
    else if (action_str == "donothing" || action_str.empty()) {
        task_info_ptr->action = ProcessTaskAction::DoNothing;
    }
    else if (action_str == "stop") {
        task_info_ptr->action = ProcessTaskAction::Stop;
    }
    else if (action_str == "clickrect") {
        task_info_ptr->action = ProcessTaskAction::ClickRect;
        if (task_json.contains("specificRect")) {
            const json::value& rect_json = task_json.at("specificRect");
            task_info_ptr->specific_rect = Rect(
                rect_json[0].as_integer(),
                rect_json[1].as_integer(),
                rect_json[2].as_integer(),
                rect_json[3].as_integer());
        }
        else {
            if (specific_rect_default.empty()) {
                m_last_error = "ClickRect task " + task_info_ptr->name + " does not have specificRect";
                return false;
            }
            task_info_ptr->specific_rect = specific_rect_default;
        }
    }
    else if (action_str == "swipetotheleft") {
        task_info_ptr->action = ProcessTaskAction::SwipeToTheLeft;
    }
    else if (action_str == "swipetotheright") {
        task_info_ptr->action = ProcessTaskAction::SwipeToTheRight;
    }
    else if (action_str == "slowlyswipetotheleft") {
        task_info_ptr->action = ProcessTaskAction::SlowlySwipeToTheLeft;
    }
    else if (action_str == "slowlyswipetotheright") {
        task_info_ptr->action = ProcessTaskAction::SlowlySwipeToTheRight;
    }
    else {
        m_last_error = "Task " + task_info_ptr->name + " have invalid action " + action_str;
        return false;
    }
    return true;
}

bool asst::TaskData::generate_match_task(
    const std::shared_ptr<asst::TaskInfo>& task_info_ptr,
    const json::value& task_json,
    const std::string& templ_name_default,
    const double& match_templ_threshold_default,
    const double& match_special_threshold_default,
    const std::pair<int, int>& mask_range_default)
{
    auto match_task_info_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(task_info_ptr);
    match_task_info_ptr->templ_name = task_json.get("template", templ_name_default);
    m_templ_required.emplace(match_task_info_ptr->templ_name);

    match_task_info_ptr->templ_threshold = task_json.get(
        "templThreshold", match_templ_threshold_default);
    match_task_info_ptr->special_threshold = task_json.get(
        "specialThreshold", match_special_threshold_default);
    if (task_json.contains("maskRange")) {
        match_task_info_ptr->mask_range = std::make_pair(
            task_json.at("maskRange")[0].as_integer(),
            task_json.at("maskRange")[1].as_integer());
    }
    else {
        match_task_info_ptr->mask_range = mask_range_default;
    }
    return true;
}

bool asst::TaskData::generate_ocr_task(
    const std::shared_ptr<asst::TaskInfo>& task_info_ptr,
    const json::value& task_json,
    const std::vector<std::string>& text_default,
    const bool& full_match_default,
    const std::unordered_map<std::string, std::string>& ocr_replace_default)
{
    auto ocr_task_info_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(task_info_ptr);
    if (task_json.contains("text")) {
        for (const json::value& text : task_json.at("text").as_array()) {
            ocr_task_info_ptr->text.emplace_back(text.as_string());
        }
    }
    else {
        if (text_default.empty()) {
            m_last_error = "Ocr task " + task_info_ptr->name + " does not have text";
            return false;
        }
        ocr_task_info_ptr->text = text_default;
    }
    ocr_task_info_ptr->full_match = task_json.get("fullMatch", full_match_default);
    if (task_json.contains("ocrReplace")) {
        for (const json::value& rep : task_json.at("ocrReplace").as_array()) {
            ocr_task_info_ptr->replace_map.emplace(rep.as_array()[0].as_string(), rep.as_array()[1].as_string());
        }
    }
    else {
        ocr_task_info_ptr->replace_map = ocr_replace_default;
    }
    return true;
}

bool asst::TaskData::generate_hash_task(
    const std::shared_ptr<asst::TaskInfo>& task_info_ptr,
    const json::value& task_json,
    const std::vector<std::string>& hashs_default,
    const int& dist_threshold_default,
    const std::pair<int, int>& mask_range_default,
    const bool& bound_default)
{
    auto hash_task_info_ptr = std::dynamic_pointer_cast<HashTaskInfo>(task_info_ptr);
    if (task_json.contains("hash")) {
        for (const json::value& hash : task_json.at("hash").as_array()) {
            hash_task_info_ptr->hashs.emplace_back(hash.as_string());
        }
    }
    else {
        if (hashs_default.empty()) {
            m_last_error = "Hash task " + task_info_ptr->name + " does not have hashs";
            return false;
        }
        hash_task_info_ptr->hashs = hashs_default;
    }
    hash_task_info_ptr->dist_threshold = task_json.get("threshold", dist_threshold_default);
    if (task_json.contains("maskRange")) {
        hash_task_info_ptr->mask_range = std::make_pair(
            task_json.at("maskRange")[0].as_integer(),
            task_json.at("maskRange")[1].as_integer());
    }
    else {
        hash_task_info_ptr->mask_range = mask_range_default;
    }
    hash_task_info_ptr->bound = task_json.get("bound", bound_default);
    return true;
}

bool asst::TaskData::parse_single(const std::string& name, const json::value& task_json)
{
    static auto to_lower = [](char c) -> char {
        return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    };
    std::shared_ptr<asst::TaskInfo> task_info_ptr = nullptr;
    if (!generate_task_algorithm(task_info_ptr, task_json.get("algorithm", "matchtemplate"))) {
        return false;
    }
    task_info_ptr->name = name;

    switch (task_info_ptr->algorithm) {
    case AlgorithmType::JustReturn:
        break;
    case AlgorithmType::MatchTemplate:
        if (!generate_match_task(task_info_ptr, task_json, name + ".png")) {
            return false;
        }
        break;
    case AlgorithmType::OcrDetect:
        if (!generate_ocr_task(task_info_ptr, task_json)) {
            return false;
        }
        break;
    case AlgorithmType::Hash:
        if (!generate_hash_task(task_info_ptr, task_json)) {
            return false;
        }
        break;
    }
    task_info_ptr->cache = task_json.get("cache", true);
    if (!generate_task_action(task_info_ptr, task_json, task_json.get("action", std::string()))) {
        return false;
    }

    task_info_ptr->max_times = task_json.get("maxTimes", INT_MAX);
    if (task_json.contains("exceededNext")) {
        const json::array& excceed_next_arr = task_json.at("exceededNext").as_array();
        for (const json::value& excceed_next : excceed_next_arr) {
            task_info_ptr->exceeded_next.emplace_back(excceed_next.as_string());
        }
    }
    else {
        task_info_ptr->exceeded_next.emplace_back("Stop");
    }
    task_info_ptr->pre_delay = task_json.get("preDelay", 0);
    task_info_ptr->rear_delay = task_json.get("rearDelay", 0);
    if (task_json.contains("reduceOtherTimes")) {
        const json::array& reduce_arr = task_json.at("reduceOtherTimes").as_array();
        for (const json::value& reduce : reduce_arr) {
            task_info_ptr->reduce_other_times.emplace_back(reduce.as_string());
        }
    }
    if (task_json.contains("roi")) {
        const json::array& area_arr = task_json.at("roi").as_array();
        int x = area_arr[0].as_integer();
        int y = area_arr[1].as_integer();
        int width = area_arr[2].as_integer();
        int height = area_arr[3].as_integer();
#ifdef ASST_DEBUG
        if (x + width > WindowWidthDefault || y + height > WindowHeightDefault) {
            m_last_error = name + " roi is out of bounds";
            return false;
        }
#endif
        task_info_ptr->roi = Rect(x, y, width, height);
    }
    else {
        task_info_ptr->roi = Rect();
    }

    if (task_json.contains("next")) {
        for (const json::value& next : task_json.at("next").as_array()) {
            task_info_ptr->next.emplace_back(next.as_string());
        }
    }
    if (task_json.contains("rectMove")) {
        const json::array& move_arr = task_json.at("rectMove").as_array();
        task_info_ptr->rect_move = Rect(
            move_arr[0].as_integer(),
            move_arr[1].as_integer(),
            move_arr[2].as_integer(),
            move_arr[3].as_integer());
    }
    else {
        task_info_ptr->rect_move = Rect();
    }

    m_all_tasks_info[name] = task_info_ptr;
    return true;
}

bool asst::TaskData::parse(const json::value& json)
{
    LogTraceFunction;

    static auto to_lower = [](char c) -> char {
        return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    };

    std::unordered_map<std::string, json::value> _waiting_json;
    for (const auto& [name, task_json] : json.as_object()) {
        int name_split = static_cast<int>(name.find("@") + 1);
        if (name_split) {
            _waiting_json.emplace(name, task_json);
            continue;
        }
        parse_single(name, task_json);
    }

    for (const auto& [name, task_json] : _waiting_json) {
        const std::string name_without_prefix = std::string(name, name.find("@") + 1);
        auto base_task_info_ptr_iter = m_all_tasks_info.find(name_without_prefix);
        if (base_task_info_ptr_iter == m_all_tasks_info.cend()) {
            parse_single(name, task_json);
            continue;
        }
        const auto& base_ptr = base_task_info_ptr_iter->second;
        std::shared_ptr<asst::TaskInfo> task_info_ptr = nullptr;
        if (task_json.contains("algorithm")) {
            if (!generate_task_algorithm(task_info_ptr, task_json.at("algorithm").as_string())) {
                return false;
            }
            if (task_info_ptr->algorithm != base_ptr->algorithm) {
#ifdef ASST_DEBUG
                Log.debug(name + " has different algorithm than " + base_ptr->name);
#endif
                parse_single(name, task_json);
                continue;
            }
        }
        else {
            switch (base_ptr->algorithm) {
            case AlgorithmType::JustReturn:
                task_info_ptr = std::make_shared<TaskInfo>();
                break;
            case AlgorithmType::MatchTemplate:
                task_info_ptr = std::make_shared<MatchTaskInfo>();
                break;
            case AlgorithmType::OcrDetect:
                task_info_ptr = std::make_shared<OcrTaskInfo>();
                break;
            case AlgorithmType::Hash:
                task_info_ptr = std::make_shared<HashTaskInfo>();
                break;
            default:
                Log.debug(name + " has unknown algorithm");
                return false;
            }
            task_info_ptr->algorithm = base_ptr->algorithm;
        }
        task_info_ptr->name = name;

        switch (task_info_ptr->algorithm) {
        case AlgorithmType::JustReturn:
            break;
        case AlgorithmType::MatchTemplate: {
            auto base_match_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(base_ptr);
            if (!generate_match_task(task_info_ptr, task_json, base_match_ptr->templ_name,
                base_match_ptr->templ_threshold, base_match_ptr->special_threshold, base_match_ptr->mask_range)) {
                return false;
            }
        } break;
        case AlgorithmType::OcrDetect: {
            auto base_ocr_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(base_ptr);
            if (!generate_ocr_task(task_info_ptr, task_json, base_ocr_ptr->text,
                base_ocr_ptr->full_match, base_ocr_ptr->replace_map)) {
                return false;
            }
        } break;
        case AlgorithmType::Hash: {
            auto base_hash_ptr = std::dynamic_pointer_cast<HashTaskInfo>(base_ptr);
            if (!generate_hash_task(task_info_ptr, task_json, base_hash_ptr->hashs,
                base_hash_ptr->dist_threshold, base_hash_ptr->mask_range, base_hash_ptr->bound)) {
                return false;
            }
        } break;
        }
        task_info_ptr->cache = task_json.get("cache", base_ptr->cache);
        if (task_json.contains("action")) {
            if (!generate_task_action(task_info_ptr, task_json, task_json.get("action", std::string()), base_ptr->specific_rect)) {
                return false;
            }
        }
        else {
            task_info_ptr->action = base_ptr->action;
        }

        task_info_ptr->max_times = task_json.get("maxTimes", base_ptr->max_times);
        if (task_json.contains("exceededNext")) {
            const json::array& excceed_next_arr = task_json.at("exceededNext").as_array();
            for (const json::value& excceed_next : excceed_next_arr) {
                task_info_ptr->exceeded_next.emplace_back(excceed_next.as_string());
            }
        }
        else {
            task_info_ptr->exceeded_next = base_ptr->exceeded_next;
        }
        task_info_ptr->pre_delay = task_json.get("preDelay", base_ptr->pre_delay);
        task_info_ptr->rear_delay = task_json.get("rearDelay", base_ptr->rear_delay);
        if (task_json.contains("reduceOtherTimes")) {
            const json::array& reduce_arr = task_json.at("reduceOtherTimes").as_array();
            for (const json::value& reduce : reduce_arr) {
                task_info_ptr->reduce_other_times.emplace_back(reduce.as_string());
            }
        }
        else {
            task_info_ptr->reduce_other_times = base_ptr->reduce_other_times;
        }
        if (task_json.contains("roi")) {
            const json::array& area_arr = task_json.at("roi").as_array();
            int x = area_arr[0].as_integer();
            int y = area_arr[1].as_integer();
            int width = area_arr[2].as_integer();
            int height = area_arr[3].as_integer();
#ifdef ASST_DEBUG
            if (x + width > WindowWidthDefault || y + height > WindowHeightDefault) {
                m_last_error = name + " roi is out of bounds";
                return false;
            }
#endif
            task_info_ptr->roi = Rect(x, y, width, height);
        }
        else {
            task_info_ptr->roi = base_ptr->roi;
        }

        if (task_json.contains("next")) {
            for (const json::value& next : task_json.at("next").as_array()) {
                task_info_ptr->next.emplace_back(next.as_string());
            }
        }
        else {
            task_info_ptr->next = base_ptr->next;
        }
        if (task_json.contains("rectMove")) {
            const json::array& move_arr = task_json.at("rectMove").as_array();
            task_info_ptr->rect_move = Rect(
                move_arr[0].as_integer(),
                move_arr[1].as_integer(),
                move_arr[2].as_integer(),
                move_arr[3].as_integer());
        }
        else {
            task_info_ptr->rect_move = base_ptr->rect_move;
        }

        m_all_tasks_info[name] = task_info_ptr;
    }

#ifdef ASST_DEBUG
    bool may_be_null = false;
    for (const auto& [name, task] : m_all_tasks_info) {
        for (const auto& next : task->next) {
            if (/*m_all_tasks_info.find(next) == m_all_tasks_info.cend()*/get(next) == nullptr) {
                m_last_error = m_last_error + name + "'s next " + next + " may be null\n";
                may_be_null = true;
            }
        }
    }
    if (may_be_null) return false;
#endif

    return true;
}
