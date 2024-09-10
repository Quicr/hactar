#pragma once

template <typename T>
struct LinkedQueueItem
{
    T data;
    LinkedQueueItem<T> *next = nullptr;
};

template <typename T>
class LinkedQueue
{
public:
    LinkedQueue() : max_size(__UINT32_MAX__), size(0)
    {
        InitQueue();
    }

    LinkedQueue(unsigned int max_size) : max_size(max_size), size(0)
    {
        InitQueue();
    }
    ~LinkedQueue()
    {
        // Empty the queue
        Clear();

        // Clear the hanging memory, by this point tail and head should be
        // pointing to the same data and so we don't need to delete tail
        delete head;

        // Set all the pointer to a nullptr
        head = nullptr;
        tail = nullptr;
    }
    bool Enqueue(T data)
    {
        if (size >= max_size)
            return false;

        // Set the data of the tail
        tail->data = data;

        // Set the next of the tail
        tail->next = new LinkedQueueItem<T>;

        // Set the pointer object to the next item
        tail = tail->next;

        size++;

        return true;
    }
    T Front()
    {
        return head->data;
    }
    void Pop()
    {
        if (head == nullptr)
            return;

        // Get next pointer
        LinkedQueueItem<T> *next = head->next;

        // Delete the memory at head
        delete head;

        // Update the head to be the next item in the linked list
        head = next;

        // Decrement the size
        size--;
    }
    bool Empty()
    {
        return !(size > 0);
    }
    unsigned int Size()
    {
        return size;
    }
    void Clear()
    {
        while (!Empty())
        {
            Pop();
        }
    }

private:
    void InitQueue()
    {
        // Set the head to a new Queue item
        head = new LinkedQueueItem<T>;

        // Initially point the tail to the head
        tail = head;
    }

    unsigned int max_size;
    unsigned int size;

    LinkedQueueItem<T> *head;
    LinkedQueueItem<T> *tail;
};