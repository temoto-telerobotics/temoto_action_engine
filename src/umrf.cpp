/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2019 TeMoto Telerobotics
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Author: Robert Valner */

#include "temoto_action_engine/umrf.h"
#include "temoto_action_engine/temoto_error.h"
#include <iostream>

Umrf::Umrf()
{}

Umrf::Umrf(const Umrf& uj)
: name_(uj.name_)
, description_(uj.description_)
, package_name_(uj.package_name_)
, suffix_(uj.suffix_)
, notation_(uj.notation_)
, effect_(uj.effect_)
, library_path_(uj.library_path_)
, parents_(uj.parents_)
, children_(uj.children_)
, id_(uj.id_)
, full_name_(uj.full_name_)
, input_parameters_(uj.input_parameters_)
, output_parameters_(uj.output_parameters_)
{}

const std::string& Umrf::getName() const
{
  return name_;
}

std::string& Umrf::getNameNc()
{
  return name_;
}

bool Umrf::setName(const std::string& name)
{
  if (!name.empty())
  {
    name_ = name;
    full_name_ = name_ + "_" + std::to_string(suffix_);
    return true;  
  }
  else
  {
    return false;
  }
}

const std::string& Umrf::getPackageName() const
{
  return package_name_;
}

const std::string& Umrf::getDescription() const
{
  return description_;
}

bool Umrf::setDescription(const std::string& description)
{
  description_ = description;
}

bool Umrf::setPackageName(const std::string& package_name)
{
  if (!package_name.empty())
  {
    package_name_ = package_name;
    return true;  
  }
  else
  {
    return false;
  }
}

const std::string& Umrf::getFullName() const
{
  return full_name_;
}

const std::string& Umrf::getLibraryPath() const
{
  return library_path_;
}

bool Umrf::setLibraryPath(const std::string& library_path)
{
  if (!library_path.empty())
  {
    library_path_ = library_path;
    return true;  
  }
  else
  {
    return false;
  }
}

const std::vector<Umrf::Relation>& Umrf::getParents() const
{
  return parents_;
}

bool Umrf::setParents(const std::vector<Umrf::Relation>& parents)
{
  if (!parents.empty())
  {
    parents_ = parents;
    return true;  
  }
  else
  {
    return false;
  }
}

void Umrf::clearParents()
{
  parents_.clear();
}

bool Umrf::addParent(const Umrf::Relation& parent)
{
  if (!parent.empty())
  {
    parents_.push_back(parent);
    return true;
  }
  else
  {
    return false;
  }
}

bool Umrf::removeParent(const Umrf::Relation& parent)
{
  auto parent_it = std::find(parents_.begin(), parents_.end(), parent);

  if (parent_it != parents_.end())
  {
    parents_.erase(parent_it);
    return true;
  }
  else
  {
    return false;
  }
}

const std::vector<Umrf::Relation>& Umrf::getChildren() const
{
  return children_;
}

bool Umrf::setChildren(const std::vector<Umrf::Relation>& children)
{
  if (!children.empty())
  {
    children_ = children;
    return true;  
  }
  else
  {
    return false;
  }
}

void Umrf::clearChildren()
{
  children_.clear();
}

bool Umrf::addChild(const Umrf::Relation& child)
{
  if (!child.empty())
  {
    children_.push_back(child);
    return true;
  }
  else
  {
    return false;
  }
}

bool Umrf::removeChild(const Umrf::Relation& child)
{
  auto child_it = std::find(children_.begin(), children_.end(), child);

  if (child_it != children_.end())
  {
    children_.erase(child_it);
    return true;
  }
  else
  {
    return false;
  }
}

const std::string& Umrf::getEffect() const
{
  return effect_;
}

std::string& Umrf::getEffectNc()
{
  return effect_;
}

bool Umrf::setEffect(const std::string& effect)
{
  if (!effect.empty())
  {
    effect_ = effect;
    return true;  
  }
  else
  {
    return false;
  }
}

const unsigned int& Umrf::getSuffix() const
{
  return suffix_;
}
bool Umrf::setSuffix(const unsigned int& suffix)
{

  suffix_ = suffix;
  full_name_ = name_ + "_" + std::to_string(suffix_);
  return true;  
}

const std::string& Umrf::getNotation() const
{
  return notation_;
}

bool Umrf::setNotation(const std::string& notation)
{
  if (!notation.empty())
  {
    notation_ = notation;
    return true;  
  }
  else
  {
    return false;
  }
}

const unsigned int& Umrf::getId() const
{
  return id_;
}
bool Umrf::setId(const unsigned int& id)
{
  id_ = id;
  return true;
}

const ActionParameters& Umrf::getInputParameters() const
{
  LOCK_GUARD_TYPE_R guard_input_params(input_params_rw_mutex_);
  return input_parameters_;
}

ActionParameters& Umrf::getInputParametersNc()
{
  LOCK_GUARD_TYPE_R guard_input_params(input_params_rw_mutex_);
  return input_parameters_;
}

bool Umrf::setInputParameters(const ActionParameters& params)
{
  LOCK_GUARD_TYPE_R guard_input_params(input_params_rw_mutex_);
  if (!params.empty())
  {
    input_parameters_ = params;
    return true;
  }
  else
  {
    return false;
  }
}

bool Umrf::setInputParameter(const ActionParameters::ParameterContainer& param_in)
{
  LOCK_GUARD_TYPE_R guard_input_params(input_params_rw_mutex_);
  return input_parameters_.setParameter(param_in);
}

const ActionParameters& Umrf::getOutputParameters() const
{
  LOCK_GUARD_TYPE_R guard_output_params(output_params_rw_mutex_);
  return output_parameters_;
}

ActionParameters& Umrf::getOutputParametersNc()
{
  LOCK_GUARD_TYPE_R guard_output_params(output_params_rw_mutex_);
  return output_parameters_;
}

bool Umrf::setOutputParameters(const ActionParameters& params)
{
  LOCK_GUARD_TYPE_R guard_output_params(output_params_rw_mutex_);
  if (!params.empty())
  {
    output_parameters_ = params;
    return true;
  }
  else
  {
    return false;
  }
}

bool Umrf::isUmrfCorrect() const
{
  return (!getName().empty()
    && !getLibraryPath().empty()
  );
}

bool Umrf::copyInputParameters(const ActionParameters& action_parameters)
{
  LOCK_GUARD_TYPE_R guard_input_params(input_params_rw_mutex_);
  input_parameters_.copyParameters(action_parameters);
  return inputParametersReceived();
}

bool Umrf::inputParametersReceived() const
{
  LOCK_GUARD_TYPE_R guard_input_params(input_params_rw_mutex_);
  bool params_received = true;
  for (const auto& input_parameter : input_parameters_)
  {
    if (!input_parameter.isRequired())
    {
      continue;
    }
    if (input_parameter.getDataSize() == 0)
    {
      params_received = false;
      break;
    }
  }
  return params_received;
}

std::ostream& operator<<( std::ostream& stream, const Umrf& umrf)
{
  stream << "  name: " << umrf.getName() << std::endl;
  stream << "  suffix: " << umrf.getSuffix() << std::endl;
  stream << "  full_name: " << umrf.getFullName() << std::endl;
  stream << "  effect: " << umrf.getEffect() << std::endl;
  stream << "  lib path: " << umrf.getLibraryPath() << std::endl;
  if (!umrf.getParents().empty())
  {
    stream << "  parents:" << std::endl;
    for (const auto& parent : umrf.getParents())
    {
      stream << "   - " << parent.getFullName() << std::endl;
    }
  }

  if (!umrf.getChildren().empty())
  {
    stream << "  children:" << std::endl;
    for (const auto& child : umrf.getChildren())
    {
      stream << "   - " << child.getFullName() << std::endl;
    }
  }

  if (!umrf.getInputParameters().empty())
  {
    stream << "  input_parameters:" << std::endl;
    for (const auto& ip : umrf.getInputParameters())
    {
      stream << "   - " 
      << "name=" << ip.getName() << "; "
      << "type=" << ip.getType() << "; "
      << "required=" << ip.isRequired() << "; "
      << "data_size=" << ip.getDataSize() << "; "
      << "allowed_data_size=" << ip.getAllowedData().size() << std::endl;
    }
  }

  if (!umrf.getOutputParameters().empty())
  {
    stream << "  output_parameters:" << std::endl;
    for (const auto& op : umrf.getOutputParameters())
    {
      stream << "   - " 
      << "name=" << op.getName() << "; "
      << "type=" << op.getType() << "; "
      << "data_size=" << op.getDataSize() << std::endl;
    }
  }

  return stream;
}

bool Umrf::updateInputParams(const Umrf& umrf_in)
{
  LOCK_GUARD_TYPE_R guard_input_params(input_params_rw_mutex_);
  try
  {
    bool parameters_updated = false;
    for (const auto& input_param_in : umrf_in.getInputParameters())
    {
      // Get the parameter
      if (input_parameters_.getParameters().find(input_param_in) == input_parameters_.end())
      {
        for (const auto& p : input_parameters_.getParameters())
        {
          std::cout << p.getName() << " ";
        }
        std::cout << std::endl;
        continue;
      }

      const ActionParameters::ParameterContainer& input_param_loc = *input_parameters_.getParameters().find(input_param_in);

      // Skip that parameter if it's not updatable
      if (!input_param_loc.isUpdatable())
      {
        continue;
      }

      // Update the parameter
      if (!setInputParameter(input_param_in))
      {
        continue;
      }
      parameters_updated = true;
    }
    return parameters_updated;
  }
  catch(TemotoErrorStack e)
  {
    throw FORWARD_TEMOTO_ERROR_STACK(e);
  }
}

bool Umrf::isEqual(const Umrf& umrf_in, bool check_updatable) const
{
  LOCK_GUARD_TYPE_R guard_input_params(input_params_rw_mutex_);
  LOCK_GUARD_TYPE_R guard_output_params(output_params_rw_mutex_);

  /*
   * Compare the general parameters
   */
  if ((name_ != umrf_in.name_) ||
      (suffix_ != umrf_in.suffix_) ||
      (notation_ != umrf_in.notation_) ||
      (effect_ != umrf_in.effect_))
  {
    return false;
  }

  /*
   * Compare graph connections
   */
  // Compare parent & children connection sizes
  if ((children_.size() != umrf_in.children_.size()) ||
      (parents_.size() != umrf_in.parents_.size()))
  {
    return false;
  }

  // Compare the connections
  for (const auto& parent_in : umrf_in.parents_)
  {
    if (std::find(parents_.begin(), parents_.end(), parent_in) == parents_.end())
    {
      return false;
    }
  }

  for (const auto& child_in : umrf_in.children_)
  {
    if (std::find(children_.begin(), children_.end(), child_in) == children_.end())
    {
      return false;
    }
  }

  /*
   * Compare the parameters
   */
  const ActionParameters& input_parameters_in = umrf_in.getInputParameters();
  const ActionParameters& output_parameters_in = umrf_in.getOutputParameters();

  if ((input_parameters_.getParameterCount() != input_parameters_in.getParameterCount()) ||
      (output_parameters_.getParameterCount() != output_parameters_in.getParameterCount()))
  {
    return false;
  }

  // Check each input parameter individually
  for (const auto& input_param : input_parameters_)
  {
    if (input_parameters_in.hasParameter(input_param.getName()))
    {
      // Check if updatability has to be controlled
      if (check_updatable)
      {
        if (!input_param.isEqualNoData(input_parameters_in.getParameter(input_param.getName())))
        {
          // Params not equal
          return false;
        }
      }
      else
      {
        if (!input_param.isEqualNoDataNoUpdate(input_parameters_in.getParameter(input_param.getName())))
        {
          // Params not equal
          return false;
        }
      }
    }
    else
    {
      // Does not have the parameter
      return false;
    }
  }

  // Check each output parameter individually
  for (const auto& output_param : output_parameters_)
  {
    if (output_parameters_in.hasParameter(output_param.getName()))
    {
      if (!output_param.isEqualNoData(output_parameters_in.getParameter(output_param.getName())))
      {
        // Params not equal
        return false;
      }
    }
    else
    {
      // Does not have the parameter
      return false;
    }
  }
  return true;
}

Umrf::Relation Umrf::asRelation() const
{
  return Umrf::Relation(getName(), getSuffix());
}

bool Umrf::requiredParentsFinished() const
{
  for (const auto& parent : parents_)
  {
    if (parent.getRequired() && !parent.getReceived())
    {
      return false;
    }
  }
  return true;
}

void Umrf::setParentReceived(const Umrf::Relation& parent)
{
  auto parent_it = std::find(parents_.begin(), parents_.end(), parent);

  if (parent_it != parents_.end())
  {
    parent_it->received_ = true;
  }
  else
  {
    throw CREATE_TEMOTO_ERROR_STACK("The parent does not exist");
  }
}