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

#include "temoto_action_engine/action_executor.h"
#include "temoto_action_engine/temoto_error.h"
#include "temoto_action_engine/messaging.h"
#include <set>
 
ActionExecutor::ActionExecutor()
{}

void ActionExecutor::start()
{
  startCleanupLoopThread();
}

void ActionExecutor::notifyFinished(const unsigned int& parent_action_id, const ActionParameters& parent_action_parameters)
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
  LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);
  /*
   * Check the UMRF graphs and execute sequential actions if necessary
   */
  try
  {
    for (auto& umrf_graph_pair : named_umrf_graphs_)
    {
      if ((umrf_graph_pair.second.checkState() != UmrfGraph::State::ACTIVE) ||
          (umrf_graph_pair.second.getChildrenOf(parent_action_id).empty()))
      {
        continue;
      }

      /*
       * Transfer the parameters from parent to child action
       */
      for (const auto& child_id : umrf_graph_pair.second.getChildrenOf(parent_action_id))
      {
        Umrf& child_umrf = umrf_graph_pair.second.getUmrfOfNonconst(child_id);
        child_umrf.copyInputParameters(parent_action_parameters);
        child_umrf.setParentReceived(umrf_graph_pair.second.getUmrfOf(parent_action_id).asRelation());
      }
      executeById(umrf_graph_pair.second.getChildrenOf(parent_action_id), umrf_graph_pair.second);
    }
  }
  catch(TemotoErrorStack e)
  {
    throw FORWARD_TEMOTO_ERROR_STACK(e);
  }
  catch(const std::exception& e)
  {
    throw CREATE_TEMOTO_ERROR_STACK(std::string(e.what()));
  }
}

bool ActionExecutor::isActive() const
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
  for (const auto& named_action_handle : named_action_handles_)
  {
    if (named_action_handle.second.getState() == ActionHandle::State::RUNNING)
    {
      return true;
    }
  }
  return false;
}

unsigned int ActionExecutor::getActionCount() const
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
  return named_action_handles_.size();
}

bool ActionExecutor::stopAndCleanUp()
{
  named_action_handles_rw_mutex_.lock();
  // Stop all actions
  for (auto& named_action_handle : named_action_handles_)
  {
    TEMOTO_PRINT("Stopping action " + named_action_handle.second.getActionName());
    named_action_handle.second.stopAction(4);
  }
  named_action_handles_rw_mutex_.unlock();

  // Wait until all actions are stopped
  TEMOTO_PRINT("Waiting for all actions to stop ...");
  while (isActive())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  // Stop the cleanup loop
  TEMOTO_PRINT("Stopping the cleanup loop ...");
  cleanup_loop_spinning_ = false;
  try
  {
    while (cleanup_loop_future_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  TEMOTO_PRINT("Action Executor is stopped.");

  return true;
}

bool ActionExecutor::startCleanupLoopThread()
{
  cleanup_loop_spinning_ = true;
  cleanup_loop_future_ = std::async( std::launch::async
                                   , &ActionExecutor::cleanupLoop
                                   , this);
  return true;
}

void ActionExecutor::cleanupLoop()
{
  while(cleanup_loop_spinning_)
  {
    {
      LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
      if (!named_action_handles_.empty())
      {
        for ( auto nah_it=named_action_handles_.begin()
            ; nah_it!=named_action_handles_.end()
            ; /* empty */)
        {
          /*
           * TODO: Handle actions that have reached to error state
           */
          if ((nah_it->second.getState() == ActionHandle::State::FINISHED) &&
              nah_it->second.futureIsReady() &&
              nah_it->second.getEffect() == "synchronous")
          {
            try
            {
              std::string error_message = nah_it->second.getFutureValue().getMessage();
              if (!error_message.empty())
              {
                std::cout << error_message << std::endl;
              }
              // Notify the graph that the node has finished
              {
                LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);
                for ( auto nug_it=named_umrf_graphs_.begin()
                    ; nug_it!=named_umrf_graphs_.end()
                    ; nug_it++)
                {
                  if (!nug_it->second.partOfGraph(nah_it->first))
                  {
                    continue;
                  }
                  nug_it->second.setNodeFinished(nah_it->first);
                }
              }
              nah_it->second.clearAction();
              //named_action_handles_.erase(nah_it++); // TODO - currently the synchronous action handle is not erased but it should be ...
              nah_it++;
            }
            catch(TemotoErrorStack e)
            {
              std::cout << e.what() << '\n';
              ++nah_it;
            }
          }
          else
          {
            ++nah_it;
          }
        }

        /*
         * Remove all graphs and associated actions that have finished
         */
        LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);
        for ( auto nug_it=named_umrf_graphs_.begin()
            ; nug_it!=named_umrf_graphs_.end()
            ; /* empty */)
        {
          if (nug_it->second.checkState() == UmrfGraph::State::FINISHED)
          {
            TEMOTO_PRINT("Graph '" + nug_it->first + "' has finished.");
            // for (const auto& action_handle_id : nug_it->second.getNodeIds())
            // {
            //   TEMOTO_PRINT("erasing " + std::to_string(action_handle_id) + " ...");
            //   named_action_handles_.erase(action_handle_id);
            // }
            named_umrf_graphs_.erase(nug_it++);
          }
          else
          {
            ++nug_it;
          }
        }
      }
      else
      {
        //TEMOTO_PRINT("no actions in executing state");
      }
    } // Lock guard scope
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }
}

bool ActionExecutor::graphExists(const std::string& graph_name)
{
  LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);
  if (named_umrf_graphs_.find(graph_name) == named_umrf_graphs_.end())
  {
    // Graph does not exist
    return false;
  }
  else
  {
    // Graph exists
    return true;
  }
}

void ActionExecutor::addUmrfGraph(const std::string& graph_name, std::vector<Umrf> umrfs_vec)
{
  try
  {
    LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);

    // Check if this UMRF graph exists
    if (graphExists(graph_name))
    {
      CREATE_TEMOTO_ERROR_STACK("UMRF graph '" + graph_name + "' is already added");
    }

    // Give each UMRF a unique ID
    for (auto& umrf_json : umrfs_vec)
    {
      umrf_json.setId(createId());
    }

    // Create an UMRF graph
    UmrfGraph ugh = UmrfGraph(graph_name, umrfs_vec);
    if (ugh.checkState() == UmrfGraph::State::UNINITIALIZED)
    {
      throw CREATE_TEMOTO_ERROR_STACK("Cannot add UMRF graph because it's uninitialized.");
    }

    named_umrf_graphs_.insert(std::pair<std::string, UmrfGraph>(graph_name, ugh));
  }
  catch(TemotoErrorStack e)
  {
    throw FORWARD_TEMOTO_ERROR_STACK(e);
  }
}

void ActionExecutor::updateUmrfGraph(const std::string& graph_name, std::vector<Umrf> umrfs_vec)
{
  LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);

  try
  {
    // Check if this UMRF graph exists
    if (!graphExists(graph_name))
    {
      throw CREATE_TEMOTO_ERROR_STACK("Could not find UMRF graph '" + graph_name + "'");
    }

    /*
     * Check if the graphs are structurally the same
     */
    const UmrfGraph& ugh = named_umrf_graphs_.at(graph_name);

    // Check the size
    if (umrfs_vec.size() != ugh.getUmrfs().size())
    {
      throw CREATE_TEMOTO_ERROR_STACK("Could not update UMRF graph '" + graph_name + "' because umrf sizes do not match.");
    }

    // Compare the UMRFs
    for (const auto& umrf_existing : named_umrf_graphs_.at(graph_name).getUmrfs())
    {
      bool umrf_found = false;
      for (const auto& umrf_in : umrfs_vec)
      {
        if (umrf_existing.isEqual(umrf_in, false))
        {
          umrf_found = true;
          break;
        }
      }
      if (!umrf_found)
      {
        throw CREATE_TEMOTO_ERROR_STACK("Could not update UMRF graph '" + graph_name 
          + "' because incoming graph does not contain UMRF '" + umrf_existing.getFullName() +"'");
      }
    }

    /*
     * Update the UMRFs in the action handles
     */
    updateActionHandles(ugh, umrfs_vec);
  }
  catch(TemotoErrorStack e)
  {
    throw FORWARD_TEMOTO_ERROR_STACK(e);
  }
}

void ActionExecutor::updateActionHandles(const UmrfGraph& ugh, const std::vector<Umrf>& umrf_vec)
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
  LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);
  try
  {
    for (const auto& umrf_in : umrf_vec)
    {
      // Get handle id
      const unsigned int& handle_id = ugh.getNodeId(umrf_in.getFullName());
      if (named_action_handles_.find(handle_id) == named_action_handles_.end())
      {
        // This handle has finished execution
        continue;
      }

      ActionHandle& ah = named_action_handles_.at(handle_id);
      ah.updateUmrf(umrf_in);
    }
  }
  catch(TemotoErrorStack e)
  {
    throw FORWARD_TEMOTO_ERROR_STACK(e);
  }
}

void ActionExecutor::modifyGraph(const std::string& graph_name, const UmrfGraphDiffs& graph_diffs)
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
  LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);

  if (!graphExists(graph_name))
  {
    TEMOTO_PRINT("Cannot modify graph '" + graph_name + "' because it does not exist.");
    return;
  }

  TEMOTO_PRINT("Received a request to modify UMRF graph '" + graph_name + "' ...");
  UmrfGraph& ugh = named_umrf_graphs_.at(graph_name);

  /*
   * Before modyfing the graph, check if the graph contains the UMRFs which are menitoned in the diffs
   */ 
  for (const auto& graph_diff : graph_diffs)
  {
    if (graph_diff.operation == UmrfGraphDiff::Operation::add_umrf)
    {
      if (ugh.partOfGraph(graph_diff.umrf.getFullName()))
      {
        throw CREATE_TEMOTO_ERROR_STACK("Cannot add UMRF '" + graph_diff.umrf.getFullName()
          + "', as it is already part of graph '" + graph_name + "'");
      }
    }
    else
    {
      if (!ugh.partOfGraph(graph_diff.umrf.getFullName()))
      {
        throw CREATE_TEMOTO_ERROR_STACK("Cannot perform operation '" + graph_diff.operation
        + "' because UMRF graph '" + graph_name + "' does not contain node named '"
        + graph_diff.umrf.getFullName() + "'");
      }
    }
  }

  /*
   * Apply the diffs
   */ 
  for (const auto& graph_diff : graph_diffs)
  {
    if (graph_diff.operation == UmrfGraphDiff::Operation::add_umrf)
    {
      TEMOTO_PRINT("Applying an '" + graph_diff.operation + "' operation to UMRF '" + graph_diff.umrf.getFullName() + "' ...");
      Umrf umrf_cpy = graph_diff.umrf;
      umrf_cpy.setId(createId());
      ugh.addUmrf(umrf_cpy);
    }
    else if (graph_diff.operation == UmrfGraphDiff::Operation::remove_umrf)
    {
      TEMOTO_PRINT("Applying an '" + graph_diff.operation + "' operation to UMRF '" + graph_diff.umrf.getFullName() + "' ...");
      unsigned int action_handle_id = ugh.removeUmrf(graph_diff.umrf);
      stopAction(action_handle_id);
    }
    else if (graph_diff.operation == UmrfGraphDiff::Operation::add_child)
    {
      TEMOTO_PRINT("Applying an '" + graph_diff.operation + "' operation to UMRF '" + graph_diff.umrf.getFullName() + "' ...");
      ugh.addChild(graph_diff.umrf);
    }
    else if (graph_diff.operation == UmrfGraphDiff::Operation::remove_child)
    {
      TEMOTO_PRINT("Applying an '" + graph_diff.operation + "' operation to UMRF '" + graph_diff.umrf.getFullName() + "' ...");
      ugh.removeChild(graph_diff.umrf);
    }
    else
    {
      throw CREATE_TEMOTO_ERROR_STACK("No such operation as " + graph_diff.operation);
    }
    TEMOTO_PRINT("Finished with the '" + graph_diff.operation + "' operation.");
  }
}

void ActionExecutor::executeUmrfGraph(const std::string& graph_name)
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
  LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);
  try
  {
    // Check if the requested graph exists
    if (!graphExists(graph_name))
    {
      throw CREATE_TEMOTO_ERROR_STACK("Cannot execute UMRF graph '" + graph_name + "' because it doesn't exist.");
    }

    // Check if the graph is in initialized state
    /*
     * TODO: Also check if the graph is in running state and could it be updated
     * i.e., does any of its actions are accepting parameters that can be updated (pvf_updatable = true)
     */
    if (named_umrf_graphs_.at(graph_name).checkState() != UmrfGraph::State::INITIALIZED)
    {
      throw CREATE_TEMOTO_ERROR_STACK("Cannot execute UMRF graph '" + graph_name + "' because it's not in initialized state.");
    }

    UmrfGraph& ugh = named_umrf_graphs_.at(graph_name);
    std::vector<unsigned int> action_ids = ugh.getRootNodes();
    executeById(action_ids, ugh, true);
  }
  catch(TemotoErrorStack e)
  {
    throw FORWARD_TEMOTO_ERROR_STACK(e);
  }
}

void ActionExecutor::stopUmrfGraph(const std::string& graph_name)
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
  LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);

  // Check if the requested graph exists
  if (named_umrf_graphs_.find(graph_name) == named_umrf_graphs_.end())
  {
    throw CREATE_TEMOTO_ERROR_STACK("Cannot stop UMRF graph '" + graph_name + "' because it doesn't exist.");
  }

  for (const auto& umrf : named_umrf_graphs_.at(graph_name).getUmrfs())
  {
    try
    {
      stopAction(umrf.getId());
    }
    catch (TemotoErrorStack e)
    {
      throw FORWARD_TEMOTO_ERROR_STACK(e);
    }
    catch (std::exception& e)
    {
      throw CREATE_TEMOTO_ERROR_STACK(e.what());
    }
  }
  named_umrf_graphs_.erase(graph_name);
}

void ActionExecutor::stopAction(unsigned int action_handle_id)
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);

  auto action_handle_it = named_action_handles_.find(action_handle_id);
  if (action_handle_it == named_action_handles_.end())
  {
    return;
  }
  try
  {
    action_handle_it->second.clearAction();
    named_action_handles_.erase(action_handle_it);
  }
  catch (TemotoErrorStack e)
  {
    throw FORWARD_TEMOTO_ERROR_STACK(e);
  }
  catch (std::exception& e)
  {
    throw CREATE_TEMOTO_ERROR_STACK(e.what());
  }
}

void ActionExecutor::executeById(const std::vector<unsigned int> ids, UmrfGraph& ugh, bool initialized_requrired)
{
  LOCK_GUARD_TYPE_R guard_handles(named_action_handles_rw_mutex_);
  LOCK_GUARD_TYPE_R guard_graphs(named_umrf_graphs_rw_mutex_);

  std::set<unsigned int> action_rollback_list;
  std::set<unsigned int> init_action_ids;
  try
  {
    /*
     * Load the action handles
     */ 
    HandleMap named_action_handles_tmp;
    for (const auto& action_id : ids)
    {
      ActionHandle ah = ActionHandle(ugh.getUmrfOf(action_id), this);
      if (ah.getState() != ActionHandle::State::INITIALIZED)
      {
        if (initialized_requrired)
        {
          throw CREATE_TEMOTO_ERROR_STACK("Cannot execute the actions because all actions were not fully initialized.");
        }
      }
      else
      {
        named_action_handles_tmp.insert(std::pair<unsigned int, ActionHandle>(ah.getHandleId(), ah));
        init_action_ids.insert(ah.getHandleId());
        action_rollback_list.insert(ah.getHandleId());
      }
    }

    // put all loaded action handles to the global named_action_handles map
    named_action_handles_.insert(named_action_handles_tmp.begin(), named_action_handles_tmp.end());

    /*
     * Load/Instantiate the actions. If there are any problems with instantiating
     * the graph, then the whole graph is rolled back (uninitialized)
     */
    for (const auto& action_id : init_action_ids)
    {
      // Instantiate the action
      try
      {
        named_action_handles_.at(action_id).instantiateAction();
        action_rollback_list.insert(action_id);
      }
      catch(TemotoErrorStack e)
      {
        ugh.setNodeError(action_id);
        throw FORWARD_TEMOTO_ERROR_STACK(e);
      } 
      catch(const std::exception& e)
      {
        ugh.setNodeError(action_id);
        throw CREATE_TEMOTO_ERROR_STACK("Cannot initialize the actions because: " 
          + std::string(e.what()));
      }
    }

    /*
     * Execute the actions. If there are any problems with executing
     * the graph, then the whole graph is rolled back
     */
    for (const auto& action_id : init_action_ids)
    {
      // Execute the action
      try
      {
        named_action_handles_.at(action_id).executeActionThread();
        action_rollback_list.insert(action_id);
        ugh.setNodeActive(action_id);
      }
      catch(TemotoErrorStack e)
      {
        ugh.setNodeError(action_id);
        throw FORWARD_TEMOTO_ERROR_STACK(e);
      } 
      catch(const std::exception& e)
      {
        ugh.setNodeError(action_id);
        throw CREATE_TEMOTO_ERROR_STACK("Cannot execute the actions because: " 
          + std::string(e.what()));
      }
    }
  }
  catch(TemotoErrorStack e)
  {
    std::cout << "Rollbacking actions" << std::endl;
    for (const auto& action_id : action_rollback_list)
    {
      named_action_handles_.at(action_id).clearAction();
      named_action_handles_.erase(action_id);
      ugh.setNodeFinished(action_id);
    }
    throw FORWARD_TEMOTO_ERROR_STACK(e);
  }
}

unsigned int ActionExecutor::createId()
{
  return action_handle_id_count_++;
}