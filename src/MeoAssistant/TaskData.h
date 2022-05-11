#pragma once

#include "AbstractConfiger.h"
#include "AsstTypes.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace asst
{
    class TaskData : public AbstractConfiger
    {
    public:
        TaskData(const TaskData&) = delete;
        TaskData(TaskData&&) = delete;

        virtual ~TaskData() = default;

        static TaskData& get_instance()
        {
            static TaskData unique_instance;
            return unique_instance;
        }
        const std::unordered_set<std::string>& get_templ_required() const noexcept;

        const std::shared_ptr<TaskInfo> get(const std::string& name) /* const */ noexcept;
        // std::shared_ptr<TaskInfo> get(std::string name);

    protected:
        TaskData() = default;
        // 生成一个 task_info_ptr, 满足 next, exceededNext 多了前缀 task_prefix, 其余子项与 base_ptr 相同
        std::shared_ptr<TaskInfo> generate_task_info(
            const std::shared_ptr<TaskInfo>& base_ptr,
            const std::string& task_prefix) const;
        bool generate_task_algorithm(
            std::shared_ptr<TaskInfo>& task_info_ptr,
            std::string algorithm_str);
        bool generate_task_action(
            const std::shared_ptr<TaskInfo>& task_info_ptr,
            const json::value& task_json,
            std::string action_str,
            const asst::Rect& specific_rect_default = Rect());
        bool generate_match_task(
            const std::shared_ptr<TaskInfo>& task_info_ptr,
            const json::value& task_json,
            const std::string& templ_name_default,
            const double& match_templ_threshold_default = asst::TemplThresholdDefault,
            const double& match_special_threshold_default = 0,
            const std::pair<int, int>& mask_range_default = {0, 0});
        bool generate_ocr_task(
            const std::shared_ptr<TaskInfo>& task_info_ptr,
            const json::value& task_json,
            const std::vector<std::string>& text_default = {},
            const bool& full_match_default = false,
            const std::unordered_map<std::string, std::string>& ocr_replace_default = {});
        bool generate_hash_task(
            const std::shared_ptr<TaskInfo>& task_info_ptr,
            const json::value& task_json,
            const std::vector<std::string>& hashs_default = {},
            const int& dist_threshold_default = 0,
            const std::pair<int, int>& mask_range_default = {0, 0},
            const bool& bound_default = true);
        bool parse_single(const std::string& name, const json::value& task_json);

        virtual bool parse(const json::value& json);

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };

    static auto& Task = TaskData::get_instance();
}
