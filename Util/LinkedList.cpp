/**
 * File: LinkedList.cpp
 *
 * Summary:
 * Here we are implementing the functions which operate on the LinkedList
 * and its nodes.
 *
 * Functions:
 * AddElement, RemoveElement, InsertElementAfter, InsertElementBefore,
 * PushHead, PullTail which all operate on linked lists.
 *
 * Origin:
 * Linked-lists are very useful for lists on which iterating can start from
 * any node, but must end at the the end only. This can be done without giving
 * the list descriptor as last node will have lastNode->next field equal to
 * null. It is also very useful for splitting lists, as that will be easier
 * than circular lists.
 *
 * Author: Shukant Pal
 * -------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * Copyright (C) 2017 - Shukant Pal
 */
#include <Util/LinkedList.h>
#include <TYPE.h>

extern "C"
{

/**
 * Function: AddElement
 *
 * Summary:
 * This function adds the list node to the given list, assuming that it is
 * non-null and is isolated. If not, the owner list and this list, both may
 * get corruption.
 *
 * Args:
 * newNode - Node to be added
 * list - List on which the operation is being done
 *
 * Version: 1
 * Since: Circuit 2.03
 * Author: Shukant Pal
 */
void AddElement(LinkedListNode *newElement, LinkedList *list)
{
	LinkedListNode *lHead = list->head;

	if(lHead != null)
	{
		LinkedListNode *lTail = list->tail;

		if(lTail != null)
		{
			newElement->prev = lTail;
			lTail->next = newElement;
		}
		else
		{
			newElement->prev = lHead;
			lHead->next = newElement;
		}

		list->tail = newElement;
	}
	else
	{
		newElement->prev = null;
		list->head = newElement;
	}

	newElement->next = null;
	++(list->count);
}

/**
 * Function: RemoveElement
 *
 * Summary:
 * This function will remove oldElement for lList, assuming that it is already
 * in the list right now. null elements are not allowed.
 *
 * Although oldElement is removed from the chain of the list, it can still be
 * used for getting access in the list because its Next & Previous pointers
 * still have the same values.
 *
 * Args:
 * newNode - Node to be added
 * list - List is on which the operation is being done
 *
 * Since: Circuit 2.01
 * Author: Shukant Pal
 */
void RemoveElement(LinkedListNode *oldElement, LinkedList *lList)
{
	if(lList->count)
	{
		LinkedListNode *oldNext = oldElement->next;
		LinkedListNode *oldPrevious = oldElement->prev;

		if(oldPrevious != null)
			oldPrevious->next = oldNext;
		else
			lList->head = oldNext;

		if(oldNext != null)
			oldNext->prev = oldPrevious;
		else
			lList->tail = oldPrevious;

		--(lList->count);

		if(lList->count == 1)
		{
			lList->head->next = null;
			lList->tail = null;
		}
		else if(lList->count == 0)
		{
			lList->head = null;
			lList->tail = null;
		}
	}
}

/**
 * Function: InsertElementAfter
 *
 * Summary:
 * This function inserts the new element after the given old element, and
 * assumes the old element to be a non-null LinkedListNode, which is participating
 * in the same list. If the given element is null or belongs to a different list
 * then the list will become corrupted.
 *
 * The new element should also be isolated and not belong to another list,
 * otherwise that list will become corrupted.
 *
 * Args:
 * oldElement - Existing element, of the same list, after which a element is to inserted
 * newElement - A isolated element, to be added to the list
 * list - List on which operation is being done
 *
 * Version: 1
 * Since: Circuit 2.03
 * Author: Shukant Pal
 */
void InsertElementAfter(LinkedListNode *old, LinkedListNode *after, LinkedList *list)
{
	after->next = old->next;
	after->prev = old;

	old->next = after;
	if(after->next != null)
		after->next->prev = after;
	else
		list->tail = after;

	++(list->count);
}

/**
 * InsertElementBefore() -
 *
 * Summary:
 * This function inserts the new element before the given old element, and
 * assumes the old element to be a non-null LinkedListNode, which is participating
 * in the same list. If the given element is null or belongs to a different list
 * then the list will become corrupted.
 *
 * The new element should also be isolated and not belong to another list,
 * otherwise that list will become corrupted.
 *
 * Args:
 * oldElement - Existing element, of the same list, before which a element is to inserted
 * newElement - A isolated element, to add to the list
 * list - List on which operation is being done
 *
 * Version: 1
 * Since: Circuit 2.03
 * Author: Shukant Pal
 */
void InsertElementBefore(LinkedListNode *oldElement, LinkedListNode *newElement, LinkedList *list)
{
	LinkedListNode *previousElement = oldElement->prev;
	if(previousElement == null)
		list->head = newElement;
	else
		previousElement->next = newElement;
	newElement->prev = previousElement;

	oldElement->prev = newElement;
	newElement->next = oldElement;

	++(list->count);
}

/**
 * Function: PushHead
 *
 * Summary:
 * Pushes the element into the list, as done in FIFO lists. It will become the
 * new head surely.
 *
 * Args:
 * LinkedListNode *newHead - the element to insert as the new head
 * LinkedList *fifoList - list on which to operate as if fifo
 *
 * Since: Circuit 2.03
 * Author: Shukant Pal
 */
void PushHead(LinkedListNode *newHead, LinkedList *fifoList)
{
	LinkedListNode *lHead = fifoList->head;

	if(lHead == null)
		newHead->next = null;
	else
	{
		newHead->next = lHead;
		lHead->prev = newHead;

		LinkedListNode *lTail = fifoList->tail;
		if(lTail == null)
		{
			fifoList->tail = newHead;
		}
	}

	fifoList->head = newHead;
	newHead->prev = null;
	++(fifoList->count);
}

/**
 * Function: PullTail
 *
 * Summary:
 * Takes the last element out of the list as done in fifo queues.
 *
 * Args:
 * LinkedList *fromList - the list from which to take out the tail
 *
 * Returns:
 * the last node of fromList, before removing it
 *
 * Since: Circuit 2.03
 * Author: Shukant Pal
 */
LinkedListNode *PullTail(LinkedList *fromList)
{
	LinkedListNode *oldTail = fromList->tail;
	LinkedListNode *oldHead = fromList->head;

	if(oldTail != null)
	{
		if(oldHead->next == oldTail)
		{
			oldHead->next = null;
			fromList->tail = null;
		}
		else
		{
			fromList->tail = oldTail->prev;
			if(fromList->tail)
				fromList->tail->next = null;
		}
	}
	else
	{
		oldTail = oldHead;
		fromList->head = null;
	}

	--(fromList->count);
	return (oldTail);
}

}
