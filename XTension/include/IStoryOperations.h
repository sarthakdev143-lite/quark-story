/**
 * @file IStoryOperations.h
 * @brief Abstract interface for advanced story operations (Drag & Drop, Transfer, Deletion)
 * 
 * This interface defines the contract for advanced story management operations.
 * Implementation is deferred to Phase 2, but the architecture is ready for extension.
 * 
 * @author Senior Architect
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace QuarkStoryExplorer {

    /**
     * @brief Represents a drag operation state
     */
    struct DragOperation {
        std::string storyId;
        std::string storyTitle;
        int sourceIndex;
        bool isValid;
        
        DragOperation() : sourceIndex(-1), isValid(false) {}
        DragOperation(const std::string& id, const std::string& title, int index)
            : storyId(id), storyTitle(title), sourceIndex(index), isValid(true) {}
    };

    /**
     * @brief Result of a story operation
     */
    struct OperationResult {
        bool success;
        std::string message;
        std::optional<std::string> errorCode;
        
        static OperationResult Success(const std::string& msg = "Operation completed successfully") {
            return { true, msg, std::nullopt };
        }
        
        static OperationResult Failure(const std::string& msg, const std::string& code) {
            return { false, msg, code };
        }
    };

    /**
     * @brief Interface for advanced story operations
     * 
     * Defines methods for drag-and-drop, story transfer, and deletion.
     * This interface follows the Strategy Pattern for extensibility.
     */
    class IStoryOperations {
    public:
        virtual ~IStoryOperations() = default;

        /**
         * @brief Initiate a drag operation
         * @param storyId The ID of the story being dragged
         * @return DragOperation state
         */
        virtual DragOperation BeginDrag(const std::string& storyId) = 0;

        /**
         * @brief Complete a drop operation
         * @param dragOp The drag operation state
         * @param targetContext The context where the story is dropped
         * @return OperationResult indicating success or failure
         */
        virtual OperationResult EndDrop(const DragOperation& dragOp, const std::string& targetContext) = 0;

        /**
         * @brief Transfer a story to an external destination
         * @param storyId The ID of the story to transfer
         * @param destination The destination identifier (file path, service endpoint, etc.)
         * @return OperationResult indicating success or failure
         */
        virtual OperationResult TransferStory(const std::string& storyId, const std::string& destination) = 0;

        /**
         * @brief Delete a story with confirmation
         * @param storyId The ID of the story to delete
         * @param force If true, bypass confirmation prompts
         * @return OperationResult indicating success or failure
         */
        virtual OperationResult DeleteStory(const std::string& storyId, bool force = false) = 0;

        /**
         * @brief Check if an operation can be performed
         * @param storyId The ID of the story
         * @param operation The operation type ("drag", "drop", "transfer", "delete")
         * @return true if the operation is allowed
         */
        virtual bool CanPerformOperation(const std::string& storyId, const std::string& operation) = 0;
    };

    /**
     * @brief Factory function to create StoryOperations instance
     * @return Unique pointer to IStoryOperations interface
     */
    std::unique_ptr<IStoryOperations> CreateStoryOperations();

} // namespace QuarkStoryExplorer
