#ifndef DEFINITIONNODE_H
#define DEFINITIONNODE_H

#include "definition_global.h"

namespace paysages {
namespace definition {

/**
 * Base class for all nodes of the definition tree.
 */
class DEFINITIONSHARED_EXPORT DefinitionNode
{
public:
    DefinitionNode(DefinitionNode* parent, const std::string &name, const std::string &type_name = "");
    virtual ~DefinitionNode();

    virtual void save(PackStream* stream) const;
    virtual void load(PackStream* stream);

    virtual void copy(DefinitionNode* destination) const;
    virtual void validate();

    inline const std::string &getName() const {return name;}
    virtual void setName(const std::string &name);

    inline const std::string &getTypeName() const {return type_name;}

    virtual Scenery* getScenery();

    inline const DefinitionNode* getParent() const {return parent;}
    inline const DefinitionNode* getRoot() const {return root;}
    inline int getChildrenCount() const {return children.size();}

    /**
     * Return a string representation of the tree (mainly for debugging purposes).
     */
    virtual std::string toString(int indent = 0) const;

    /**
     * Apply a diff to the internal value of this node.
     *
     * All internal node modifications should be done using this method, to be reversible.
     *
     * If *backward* is true, the diff will be reversed, instead of applied.
     *
     * Return true if the diff could be applied.
     */
    virtual bool applyDiff(const DefinitionDiff *diff, bool backward=false);

protected:
    void addChild(DefinitionNode* child);
    void removeChild(DefinitionNode* child);
    virtual DefinitionNode *findChildByName(const std::string name);

    /**
     * Get the size in bytes this child will consume when serialized to a stream.
     *
     * Return -1 if it can't be known. In this case, the saving will be done in a temporary
     * stream to know the exact size, which will not be very efficient.
     */
    int getStreamSize() const;

private:
    DefinitionNode* parent;
    DefinitionNode* root;
    std::string type_name;
    std::string name;
    std::vector<DefinitionNode*> children;
};

}
}

#endif // DEFINITIONNODE_H
