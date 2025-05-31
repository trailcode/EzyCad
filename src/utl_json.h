#pragma once

#include <nlohmann/json.hpp>

class gp_Pnt2d;
class gp_Pnt;
class gp_Dir;
class gp_Pln;

nlohmann::json to_json(const gp_Pnt2d& point);
nlohmann::json to_json(const gp_Pnt& point);
nlohmann::json to_json(const gp_Dir& direction);
nlohmann::json to_json(const gp_Pln& pln);

gp_Pnt2d from_json_pnt2d(const nlohmann::json& j);
gp_Pnt   from_json_pnt(const nlohmann::json& j);
gp_Dir   from_json_dir(const nlohmann::json& j);
gp_Pln   from_json_pln(const nlohmann::json& j);
