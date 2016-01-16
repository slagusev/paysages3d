#ifndef DEFINITIONWATCHER_H
#define DEFINITIONWATCHER_H

#include "definition_global.h"

#include <string>

namespace paysages {
namespace definition {

/**
 * Base class for watchers of the definition tree.
 *
 * Watchers will be registered in DiffManager to receive DefinitionDiff objects.
 */
class DEFINITIONSHARED_EXPORT DefinitionWatcher {
  public:
    DefinitionWatcher();
    virtual ~DefinitionWatcher();

    /**
     * Abstract method called when a node changed.
     *
     * *parent* is the node that is watched (useful if *node* is a sub-node).
     */
    virtual void nodeChanged(const DefinitionNode *node, const DefinitionDiff *diff, const DefinitionNode *parent);

  protected:
    /**
     * Start watching a path in a definition tree.
     */
    void startWatching(const DefinitionNode *root, const string &path, bool init_diff = true);

    /**
     * Abstract convenience to receive integer node changes.
     */
    virtual void intNodeChanged(const string &path, int new_value, int old_value);

    /**
     * Abstract convenience to receive float node changes.
     */
    virtual void floatNodeChanged(const string &path, double new_value, double old_value);
};
}
}

#endif // DEFINITIONWATCHER_H
